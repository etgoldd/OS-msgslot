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
  if (argc != 4) {
    printf("Usage: %s <device file> <channel id> <message>\n", argv[0]);
    exit(-1);
  }

  file_desc = open( "/dev/"DEVICE_FILE_NAME, O_RDWR );
  if( file_desc < 0 ) {
    printf ("Can't open device file: %s\n", DEVICE_FILE_NAME);
    exit(-1);
  }

  ret_val = ioctl(file_desc, MSG_SLOT_CHANNEL, atoi(argv[2]));
  ret_val = write(file_desc, argv[3], strlen(argv[3]) );
  close(file_desc);
  return 0;
}
