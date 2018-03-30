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
MODULE_DESCRIPTION("send_and_recv()");
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

int
one_send(struct sarvec *vec)
{
  int err;
  struct msghdr msg;
  struct socket *sock = 0;
  struct iovec iov;
  struct sockaddr_storage address;

  // cook up a fake system msghdr for sock_sendmsg().
  err = import_single_range(WRITE, vec->data, vec->len, &iov, &msg.msg_iter);
  if(err){
    printk(KERN_INFO "sar_send import_single_range failed\n");
    return -EINVAL;
  }

  msg.msg_name = NULL;
  msg.msg_namelen = 0;
  msg.msg_control = NULL;
  msg.msg_controllen = 0;
  msg.msg_iocb = 0;
  msg.msg_flags = 0; // could be MSG_DONTWAIT

  if(vec->name != 0){
    if(vec->namelen > sizeof(address)){
      printk(KERN_INFO "sar_send name too long\n");
      return -EINVAL;
    }
    if(copy_from_user(&address, vec->name, vec->namelen) != 0){
      printk(KERN_INFO "sar_send copy_from_user failed\n");
      return -EINVAL;
    }
    msg.msg_name = (struct sockaddr*) &address;
    msg.msg_namelen = vec->namelen;
  }

  sock = sockfd_lookup(vec->fd, &err);
  if(!sock){
    printk(KERN_INFO "sockfd_lookup(%d) failed\n", vec->fd);
    return -EINVAL;
  }

  err = sock_sendmsg(sock, &msg);
  sockfd_put(sock);

  if(err < 0){
    printk(KERN_INFO "sockfd_sendmsg failed %d\n", err);
    return -EINVAL;
  }

  return 0;
}

int
one_recv(struct sarvec *vec)
{
  int err;
  struct msghdr msg;
  struct socket *sock = 0;
  struct iovec iov;
  struct sockaddr_storage address;
  int flags = 0;

  // cook up a fake system msghdr for sock_recvmsg().
  err = import_single_range(READ, vec->data, vec->len, &iov, &msg.msg_iter);
  if(err){
    printk(KERN_INFO "sar_recv import_single_range failed\n");
    return -EINVAL;
  }

  msg.msg_name = vec->name  ? (struct sockaddr*)&address : NULL;
  msg.msg_namelen = 0;
  msg.msg_control = NULL;
  msg.msg_controllen = 0;
  msg.msg_iocb = 0;
  msg.msg_flags = 0;

  sock = sockfd_lookup(vec->fd, &err);
  if(!sock){
    printk(KERN_INFO "sockfd_lookup(%d) failed\n", vec->fd);
    return -EINVAL;
  }

  err = sock_recvmsg(sock, &msg, flags);
  sockfd_put(sock);

  // the return value.
  if(err < 0){
    vec->len = 0;
  } else {
    vec->len = iov_iter_count(&msg.msg_iter);
  }
  printk(KERN_INFO "sock_recvmsg() -> %d iter_count %ld\n", err, vec->len);

  if(err < 0){
    printk(KERN_INFO "sockfd_recvmsg failed %d\n", err);
    return -EINVAL;
  }

  if(vec->name != 0){
    if(vec->namelen > sizeof(address)){
      printk(KERN_INFO "sar_recv name too long\n");
      return -EINVAL;
    }
    if(copy_to_user(vec->name, &address, vec->namelen) != 0){
      printk(KERN_INFO "sar_recv copy_to_user failed\n");
      return -EINVAL;
    }
  }

  return 0;
}

static ssize_t
sar_write(struct file *filp, const char *buffer, size_t len, loff_t *off)
{
  int i;
  struct sarvec vec;
  struct sarargs args;

  if(copy_from_user(&args, buffer, sizeof(args)) != 0){
    printk(KERN_INFO "sar_write copy_from_user failed\n");
    return -EINVAL;
  }

  for(i = 0; i < args.nsend; i++){
    if(copy_from_user(&vec, args.sendvec+i, sizeof(vec)) != 0){
      printk(KERN_INFO "sar_write copy_from_user failed\n");
      return -EINVAL;
    }
    one_send(&vec);
  }

  for(i = 0; i < args.nrecv; i++){
    if(copy_from_user(&vec, args.recvvec+i, sizeof(vec)) != 0){
      printk(KERN_INFO "sar_write copy_from_user failed\n");
      return -EINVAL;
    }
    one_recv(&vec);
  }

  // XXX how to indicate read or write error?
  return len; // XXX
}

static struct file_operations fops = {
  .read = sar_read,
  .write = sar_write, 
  .open = sar_open,
  .release = sar_release
};

static int Major;
 
static int __init sar_init(void){
   Major = register_chrdev(0, "sar", &fops);
   if(Major < 0){
     printk(KERN_ALERT "register_chrdev failed\n");
     return Major;
   }
   printk(KERN_INFO "mknod /dev/%s c %d 0\n", "sar", Major);

   return 0;
}
 
static void __exit sar_exit(void){
   unregister_chrdev(Major, "sar");
}
 
module_init(sar_init);
module_exit(sar_exit);
