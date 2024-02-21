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

MODULE_LICENSE("GPL");

//Our custom definitions of IOCTL operations
#include "message_slot.h"

// Declarations for self.
access_ok( buffer, length );


// used to prevent concurent access into the same device
static int dev_open_flag = 0;

// The message the device will give when asked
static unsigned int buffer_lengths[256];
static char message_buffers[256][BUF_LEN];

//Do we need to encrypt?
static int encryption_flag = 0;

//================== DEVICE FUNCTIONS ===========================
static int device_open( struct inode* inode,
                        struct file*  file )
{
  printk("Invoking device_open(%p)\n", file);
  file->private_data = (void*)iminor(inode);

  return SUCCESS;
}

//---------------------------------------------------------------
static int device_release( struct inode* inode,
                           struct file*  file)
{
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
  // Checking if a channel has been set.
  if (file->private_data == NULL) {
    return -EINVAL;
  }

  // Checking if the relevant buffer has a message
  unsigned int index = (int)file->private_data;
  if (buffer_lengths[index] == 0) {
    return -EWOULDBLOCK;
  }

  // Validating that the buffer is long enough for the message.
  if (length < buffer_lengths[index]) {
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
  if( copy_to_user( buffer, message_buffers[index], length ) == 0 ) {
    return length;
  }
  return -EINVAL;
}

//---------------------------------------------------------------
// a process which has already opened
// the device file attempts to write to it
static ssize_t device_write( struct file*       file,
                             const char __user* buffer,
                             size_t             length,
                             loff_t*            offset)
{
  // Checking if a channel has been set
  if (file->private_data == NULL) {
    return -EINVAL;
  }
  int index = (int)file->private_data;
  if (length > BUF_LEN || length == 0) {
    return -EMSGSIZE;
  }
  // Checking the buffer validity
  if (buffer == NULL) {
    return -EINVAL;
  }
  if (!access_ok(buffer)) {
    return -EFAULT;
  }
  return (buffer_lengths[index] = copy_from_user(message_buffers[(int)file->private_data], buffer, length)));
}

//----------------------------------------------------------------
static long device_ioctl( struct   file* file,
                          unsigned int   ioctl_command_id,
                          unsigned long  ioctl_param )
{

  // Switch according to the ioctl called
  if(IOCTL_SET_CHNL != ioctl_command_id ) {
    return SUCCESS;
    // Get the parameter given to ioctl by the process
  }
  if (ioctl_param > 255) {
    return -EINVAL;
  }
  file->private_data = ioctl_param;
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

  // Register driver capabilities. Obtain major num
  rc = register_chrdev( MAJOR_NUM, DEVICE_RANGE_NAME, &Fops );

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
