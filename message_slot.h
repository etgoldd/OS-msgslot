#ifndef CHARDEV_H
#define CHARDEV_H

#include <linux/ioctl.h>

// The major device number.
// We don't rely on dynamic registration
// any more. We want ioctls to know this
// number at compile time.
//#define MAJOR_NUM 244
#define MAJOR_NUM 235


// Set the message of the device driver
#define MSG_SLOT_CHANNEL _IOW(MAJOR_NUM, 0, unsigned long)

#define BUF_LEN 128
#define DEVICE_FILE_NAME "msgslot"
#define SUCCESS 0
#define FIND_CREAT 1

// Channel structure
typedef struct channel_s {
  unsigned int channel_id;
  char message[BUF_LEN];
  unsigned int message_length;
} channel_t;

// Channel data structure
typedef struct node_channel_s {
  channel_t channel;
  struct node_channel_s* next;
} node_channel_t;

#endif
