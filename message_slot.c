#include "message_slot.h"
#include <sys/ioctl.h>
#include <linux/fd.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#define MSG_SLOT_CHANNEL _IOW(MESSAGE_SLOT, 0, unsigned long)



int main(int argc, char *argv[]) {
    return 0;
}