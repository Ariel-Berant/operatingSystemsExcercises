#ifndef CHARDEV_H
#define CHARDEV_H

#include <linux/ioctl.h>

// The major device number.
// We don't rely on dynamic registration
// any more. We want ioctls to know this
// number at compile time.
#define MAJOR_NUM 235

// Set the message of the device driver
#define MSG_SLOT_CHANNEL _IOW(MAJOR_NUM, 0, unsigned long)

#define DEVICE_RANGE_NAME "char_dev"
#define BUF_LEN 128 // message max len = 128 bytes
#define DEVICE_FILE_NAME "simple_char_dev"
#define SUCCESS 0

struct channel
{
    unsigned int channel_id;
    char *message;
    unsigned int msg_len;
    channel *next;
};

typedef struct channel channel;

#endif
