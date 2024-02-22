#include "message_slot.h"

#include <fcntl.h>	/* open */
#include <unistd.h>     /* exit */
#include <sys/ioctl.h>  /* ioctl */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
  int file_desc;
  int ret_val;
  char buf[BUF_LEN];

  if (argc != 3) {
    printf("Usage: %s <device file> <channel id>\n", argv[0]);
    exit(-1);
  }

  file_desc = open( "/dev/"DEVICE_FILE_NAME, O_RDWR );
  if( file_desc < 0 ) {
    perror("Can't open device file: "DEVICE_FILE_NAME"\n");
    exit(-1);
  }

  ret_val = ioctl(file_desc, MSG_SLOT_CHANNEL, atoi(argv[2]));

  if (ret_val < 0) {
    perror("Error setting channel \n");
    exit(-1);
  }

  ret_val = read(file_desc, buf, BUF_LEN);

  if (ret_val < 0) {
    perror("Error reading from device file: "DEVICE_FILE_NAME"\n");
    exit(-1);
  }
  write(1, buf, ret_val); // write to stdout (file descriptor 1)
  close(file_desc);
  return 0;
}
