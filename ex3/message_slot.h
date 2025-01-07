#ifndef CHARDEV_H
#define CHARDEV_H

#include <linux/ioctl.h>
#include <linux/errno.h>

// The major device number.
// We don't rely on dynamic registration
// any more. We want ioctls to know this
// number at compile time.
#define MAJOR_NUM 235

// Set the message of the device driver
#define MSG_SLOT_CHANNEL _IOW(MAJOR_NUM, 0, unsigned long)

#define DEVICE_RANGE_NAME "message_slot"
#define BUF_LEN 128 // message max len = 128 bytes
#define DEVICE_FILE_NAME "message_slot"
#define SUCCESS 0

#endif

struct slot_channel
{
    unsigned int channel_id;
    char *message;
    unsigned int msg_len;
    struct slot_channel *next;
};

typedef struct slot_channel slot_channel;

