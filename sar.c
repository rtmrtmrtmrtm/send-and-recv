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

struct sarvec {
  int fd;
  struct msghdr *msg;
};

static ssize_t
sar_read(struct file *filp, char *buffer, size_t length, loff_t *offset)
{
  return 0;
}

static ssize_t
sar_write(struct file *filp, const char *buffer, size_t len, loff_t *off)
{
  int err;
  struct socket *sock;
  struct sarvec vec;

  if(len != sizeof(struct sarvec))
    return -EINVAL;

  if(copy_from_user(&vec, buffer, sizeof(vec)))
    return -EINVAL;
  printk(KERN_INFO "sar_write %d %p\n", vec.fd, vec.msg);

  sock = sockfd_lookup(vec.fd, &err);
  if(!sock){
    printk(KERN_INFO "sockfd_lookup(%d) failed\n", vec.fd);
    return -EINVAL;
  }

  // convert user_msghdr to msghdr
  // copy_msghdr_from_user(&sysmsg, umsg, NULL, &iov)
  // as in ___sys_sendmsg() in net/socket.c

  err = sock_sendmsg(sock, vec.msg);
  sockfd_put(sock);

  if(err != 0){
    printk(KERN_INFO "sockfd_sendmsg failed\n");
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
