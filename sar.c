/**
 * @file    sar.c
 * @author  Robert Morris
 * @date    March 29 2018
 * @version 0.1
 * @brief  big_write_read()
*/

/*
 * Thank you, Derek Molloy, for the initial example source.
 * http://derekmolloy.ie/writing-a-linux-kernel-module-part-1-introduction/
 */
 
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/net.h>
#include <linux/file.h>
#include <linux/uaccess.h>
#include "sarvec.h"
 
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Robert Morris");
MODULE_DESCRIPTION("big_write_read()");
MODULE_VERSION("0.1");

static int
sar_open(struct inode *ii, struct file *filp)
{
   try_module_get(THIS_MODULE);
   return 0;
}

static int
sar_release(struct inode *ii, struct file *filp)
{
  module_put(THIS_MODULE);
  return 0;
}

static ssize_t
sar_read(struct file *filp, char *buffer, size_t length, loff_t *offset)
{
  return 0;
}

static ssize_t
sar_write(struct file *filp, const char *buffer, size_t len, loff_t *off)
{
  int err;
  struct sarvec vec;
  struct msghdr msg;
  struct socket *sock = 0;
  struct iovec iov;
  struct sockaddr_storage address;

  if(len != sizeof(struct sarvec)){
    printk(KERN_INFO "sar_write bad length\n");
    return -EINVAL;
  }

  if(copy_from_user(&vec, buffer, sizeof(vec)) != 0){
    printk(KERN_INFO "sar_write copy_from_user failed\n");
    return -EINVAL;
  }
  printk(KERN_INFO "sar_write %d %p %ld\n", vec.fd, vec.data, vec.len);

  // cook up a fake system msghdr for sock_sendmsg().
  err = import_single_range(WRITE, vec.data, vec.len, &iov, &msg.msg_iter);
  if(err){
    printk(KERN_INFO "sar_write import_single_range failed\n");
    return -EINVAL;
  }

  msg.msg_name = NULL;
  msg.msg_control = NULL;
  msg.msg_controllen = 0;
  msg.msg_namelen = 0;
  msg.msg_flags = 0; // could be MSG_DONTWAIT

  if(vec.name != 0){
    if(vec.namelen > sizeof(address)){
      printk(KERN_INFO "sar_write name too long\n");
      return -EINVAL;
    }
    if(copy_from_user(&address, vec.name, vec.namelen) != 0){
      printk(KERN_INFO "sar_write copy_from_user failed\n");
      return -EINVAL;
    }
    msg.msg_name = (struct sockaddr*) &address;
    msg.msg_namelen = vec.namelen;
  }

  sock = sockfd_lookup(vec.fd, &err);
  if(!sock){
    printk(KERN_INFO "sockfd_lookup(%d) failed\n", vec.fd);
    return -EINVAL;
  }

  err = sock_sendmsg(sock, &msg);
  sockfd_put(sock);

  if(err < 0){
    printk(KERN_INFO "sockfd_sendmsg failed %d\n", err);
    return -EINVAL;
  }

  return sizeof(vec);
}

static struct file_operations fops = {
  .read = sar_read,
  .write = sar_write, 
  .open = sar_open,
  .release = sar_release
};

static int Major;
 
static int __init sar_init(void){
   printk(KERN_INFO "Hello from sar.ko\n");
   Major = register_chrdev(0, "sar", &fops);
   if(Major < 0){
     printk(KERN_ALERT "register_chrdev failed\n");
     return Major;
   }
   printk(KERN_INFO "mknod /dev/%s c %d 0\n", "sar", Major);

   return 0;
}
 
static void __exit sar_exit(void){
   printk(KERN_INFO "Goodbye from sar.ko\n");
   unregister_chrdev(Major, "sar");
}
 
module_init(sar_init);
module_exit(sar_exit);
