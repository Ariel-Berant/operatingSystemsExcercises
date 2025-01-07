#include "message_slot.h"

#include <fcntl.h>     /* open */
#include <unistd.h>    /* exit */
#include <sys/ioctl.h> /* ioctl */
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv[])
{
    if (argc != 3)
    {
        perror("Usage: <device_file> <ioctl_param>\n");
        exit(1);
    }

    char *device_file = argv[1], *errorStr, message[BUF_LEN];
    if (message == NULL)
    {
        perror("Memory allocation failed\n");
        exit(1);
    }
    
    int file_desc;

    file_desc = open(device_file, O_RDWR);
    if (file_desc < 0)
    {
        sprintf(errorStr, "Can't open device file: %s\n", device_file);
        perror(errorStr);
        exit(1);
    }

    unsigned int message_channel_id = atoi(argv[2]);
    int ret_val;

    ret_val = ioctl(file_desc, MSG_SLOT_CHANNEL, message_channel_id);
    if (ret_val < 0)
    {
        perror("ioctl failed\n");
        exit(1);
    }

    ret_val = read(file_desc, message, BUF_LEN); // if not failed, ret_val is the number of bytes read
    if (ret_val < 0)
    {
        perror("read failed\n");
        exit(1);
    }

    write(stdout, message, ret_val);
    free(message);

    close(file_desc);
    exit(0);
}
