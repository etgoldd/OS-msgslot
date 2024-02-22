// Declare what kind of code we want
// from the header files. Defining __KERNEL__
// and MODULE allows us to access kernel-level
// code not usually available to userspace programs.
#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE


#include <linux/kernel.h>   /* We're doing kernel work */
#include <linux/module.h>   /* Specifically, a module */
#include <linux/fs.h>       /* for register_chrdev */
#include <linux/uaccess.h>  /* for get_user and put_user */
#include <linux/string.h>   /* for memset. NOTE - not string.h!*/
#include <linux/slab.h>     /* for kmalloc */
MODULE_LICENSE("GPL");

//Our custom definitions of IOCTL operations
#include "message_slot.h"


//================== DATA STRUCTURES ============================

static node_channel_t* msgslot_files[257];

static node_channel_t* create_node(unsigned int channel_id) {
  node_channel_t* node = (node_channel_t*)kmalloc(sizeof(node_channel_t), GFP_KERNEL);
  if (NULL == node) {
    return NULL;
  }
  node->channel.channel_id = channel_id;
  node->channel.message_length = 0;
  node->next = NULL;
  return node;
}

static node_channel_t* find(node_channel_t* root, unsigned int channel_id, int create) {
  if (NULL == root) {
    if (create) {
      root = create_node(channel_id);
    }
    return root;
  }

  while (NULL != root->next && root->channel.channel_id != channel_id) {
    root = root->next;
  }
  if (root->channel.channel_id == channel_id) {
    return root;
  }
  if (create) {
    root->next = create_node(channel_id);
  }
  return root->next;
}

//================== DEVICE FUNCTIONS ===========================
static int device_open( struct inode* inode,
                        struct file*  file )
{
  int minor_num = iminor(inode);
  printk("Invoking device_open(%p)\n", file);
  file->private_data = msgslot_files[minor_num];
  // default channel is invalid channel 0.
  return SUCCESS;
}

//---------------------------------------------------------------
static int device_release( struct inode* inode,
                           struct file*  file)
{
  int minor_num = iminor(inode);
  node_channel_t* channel = msgslot_files[minor_num]->next;
  node_channel_t* next_channel;
  while (NULL != channel) {
    next_channel = channel->next;
    kfree(channel);
    channel = next_channel;
  }
  file->private_data = NULL;
  return SUCCESS;
}

//---------------------------------------------------------------
// a process which has already opened
// the device file attempts to read from it
static ssize_t device_read( struct file* file,
                            char __user* buffer,
                            size_t       length,
                            loff_t*      offset )
{
  node_channel_t* channel = (node_channel_t*)file->private_data;
  prink("Invoking device read(%p)\n", file);
  // Checking if a channel has been set.
  if (channel->channel.channel_id == 0) {
    return -EINVAL;
  }

  // Checking if the relevant buffer has a message
  if (channel->channel.message_length == 0) {
    return -EWOULDBLOCK;
  }

  // Validating that the buffer is long enough for the message.
  if (length < channel->channel.message_length) {
    return -ENOSPC;
  }

  // Checking buffer validity.
  if( buffer == NULL ) {
    return -EINVAL;
  }
  if ( !access_ok( buffer, length ) ) {
    return -EFAULT;
  }

  // Copying the message to the buffer.
  if ( copy_to_user(buffer, channel->channel.message, channel->channel.message_length) == 0 ) {
    return -EINVAL;
  }

  return length;
}

//---------------------------------------------------------------
// a process which has already opened
// the device file attempts to write to it
static ssize_t device_write( struct file*       file,
                             const char __user* buffer,
                             size_t             length,
                             loff_t*            offset)
{
  node_channel_t* channel = (node_channel_t*)file->private_data;
  printk("Invoking device_write(%p)\n", file);
  // Checking if a channel has been set
  if (channel->channel.channel_id == 0) {
    return -EINVAL;
  }
  // Checking the message length
  if (length > BUF_LEN || length == 0) {
    return -EMSGSIZE;
  }
  // Checking the buffer validity
  if (buffer == NULL) {
    return -EINVAL;
  }
  if (!access_ok(buffer, length)) {
    return -EFAULT;
  }

  // Copying the message to the buffer
  if (copy_from_user(channel->channel.message, buffer, length) == 0) {
    return -EINVAL;
  }
  channel->channel.message_length = length;
  return length;
}

//----------------------------------------------------------------
static long device_ioctl( struct   file* file,
                          unsigned int   ioctl_command_id,
                          unsigned long  ioctl_param )
{
  node_channel_t* channel;
  printk("Invoking ioctl, requesting channel: %d\n", ioctl_param);
  // Switch according to the ioctl called
  if(MSG_SLOT_CHANNEL != ioctl_command_id ) {
    return -EINVAL;
  }
  if (ioctl_param == 0) {
    return -EINVAL;
  }
  channel = find(channels, ioctl_param, FIND_CREAT);
  file->private_data = channel;
  return SUCCESS;
}

//==================== DEVICE SETUP =============================

// This structure will hold the functions to be called
// when a process does something to the device we created
struct file_operations Fops = {
  .owner	  = THIS_MODULE, 
  .read           = device_read,
  .write          = device_write,
  .open           = device_open,
  .unlocked_ioctl = device_ioctl,
  .release        = device_release,
};

//---------------------------------------------------------------
// Initialize the module - Register the character device
static int __init simple_init(void)
{
  int rc = -1;
  int i;
  // Register driver capabilities. Obtain major num
  rc = register_chrdev( MAJOR_NUM, DEVICE_RANGE_NAME, &Fops );

  for (i = 0; i < 257; i++) {
    msgslot_files[i] = create_node(0);
  }
  // Negative values signify an error
  if( rc < 0 ) {
    printk( KERN_ALERT "%s registraion failed for  %d\n",
                       DEVICE_FILE_NAME, MAJOR_NUM );
    return rc;
  }

  return 0;
}

//---------------------------------------------------------------
static void __exit simple_cleanup(void)
{
  // Unregister the device
  // Should always succeed
  unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);
}

//---------------------------------------------------------------
module_init(simple_init);
module_exit(simple_cleanup);

//========================= END OF FILE =========================
