#include "message_slot.h"

#include <fcntl.h>     /* open */
#include <unistd.h>    /* exit */
#include <sys/ioctl.h> /* ioctl */
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv[])
{
    if (argc != 4)
    {
        perror("Usage: <device_file> <ioctl_command> <ioctl_param>\n");
        exit(1);
    }

    char *device_file = argv[1], *errorStr;
    int file_desc;

    file_desc = open(device_file, O_RDWR);
    if (file_desc < 0)
    {
        sprintf(errorStr, "Can't open device file: %s\n", device_file);
        perror(errorStr);
        exit(1);
    }

    unsigned int message_channel_id = atoi(argv[2]), len = (unsigned int)strlen(argv[3]);
    int ret_val;

    ret_val = ioctl(file_desc, MSG_SLOT_CHANNEL, message_channel_id);
    if (ret_val < 0)
    {
        perror("ioctl failed\n");
        exit(1);
    }
    
    ret_val = write(file_desc, argv[3], len);
    if (ret_val < 0)
    {
        perror("write failed\n");
        exit(1);
    }

    close(file_desc);
    exit(0);
}
