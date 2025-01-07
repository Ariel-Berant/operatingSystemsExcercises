// insert module from ex3/recitationCode/chardev.c here, modify according to the instructions
//  major number = 235 - done
//  add <linux/slab.h> for kmalloc - done
//  Declare what kind of code we want
//  from the header files. Defining __KERNEL__
//  and MODULE allows us to access kernel-level
//  code not usually available to userspace programs.
#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE

#include <linux/types.h>   /* size_t and more*/
#include <linux/kernel.h>  /* We're doing kernel work */
#include <linux/module.h>  /* Specifically, a module */
#include <linux/fs.h>      /* for register_chrdev */
#include <linux/uaccess.h> /* for get_user and put_user */
#include <linux/string.h>  /* for memset. NOTE - not string.h!*/
#include <linux/slab.h>    /* for kmalloc */
#include <linux/unistd.h>
#include <linux/errno.h>

MODULE_LICENSE("GPL");

// Our custom definitions of IOCTL operations
#include "message_slot.h"

struct chardev_info
{
    slot_channel **channels;
};

typedef struct chardev_info chardev_info;

static chardev_info device_info;

//================== DEVICE FUNCTIONS ===========================
static int device_open(struct inode *inode,
                       struct file *file)
{
    int minor = iminor(inode);
    if (minor < 0 || minor > 255) // check if minor is valid
    {
        return -EINVAL;
    }

    if (device_info.channels[minor] == NULL) // check if channel exists, if not create it
    {
        device_info.channels[minor] = kmalloc(sizeof(slot_channel), GFP_KERNEL);
        if (device_info.channels[minor] == NULL)
        {
            return -ENOMEM;
        }
        printk(KERN_INFO "Slot number %d created\n", minor);
        device_info.channels[minor]->channel_id = 0;
        device_info.channels[minor]->msg_len = 0;
        device_info.channels[minor]->next = NULL;
        device_info.channels[minor]->message = NULL;
        file->private_data = (void *)0;
    }

    // if slot already exists, no need to create it
    return SUCCESS;
    // ensure code is ok and (maybe) works?
}

//---------------------------------------------------------------
static int device_release(struct inode *inode,
                          struct file *file) // free even if not allocated, check if ok
{
    int minor = iminor(inode);
    if (minor < 0 || minor > 255)
    {
        return -EINVAL;
    }

    if (device_info.channels[minor] != NULL)
    {
        slot_channel *temp = device_info.channels[minor];
        while (temp != NULL) // free all channels - free current, then go to next
        {
            slot_channel *next = temp->next;
            kfree(temp->message);
            kfree(temp);
            temp = next;
        }

        device_info.channels[minor] = NULL;
    }

    return SUCCESS;
}

//---------------------------------------------------------------
// a process which has already opened
// the device file attempts to read from it
// in ex. 3, message length is limited to 128 bytes
static ssize_t device_read(struct file *file,
                           char __user *buffer,
                           size_t length,
                           loff_t *offset)
{
    int i;
    slot_channel *channel = NULL;
    unsigned int channel_num = (unsigned int)(unsigned long)(file->private_data);
    if (channel_num == 0) // check if channel set on fd
    {
        printk(KERN_INFO "Channel number is 0 somehow");
        return -EINVAL;
    }
    channel = device_info.channels[iminor(file->f_inode)]; // get slot
    if (channel == NULL)
    {
        printk(KERN_INFO "Slot is not open");
        return -EINVAL;
    }
    while (channel != NULL)
    {
        if (channel->channel_id == channel_num)
        {
            if (channel->msg_len > length) // check if message is too long
            {
                return -ENOSPC;
            }
            if (channel->message == NULL) // check if a message exists on the channel
            {
                return -EWOULDBLOCK;
            }
            if (buffer == NULL) // check if buffer is valid
            {
                printk(KERN_INFO "Got NULL buffer");
                return -EINVAL;
            }

            for (i = 0; i < channel->msg_len; i++)
            {
                put_user(channel->message[i], &buffer[i]);
            }
            printk(KERN_INFO "Read from slot number %d channel number %d, message: %s\n", iminor(file->f_inode), channel_num, channel->message);
            printk(KERN_INFO "Slot number is %d first channel num is %d channel to read\\write from is %d", iminor(file->f_inode), device_info.channels[iminor(file->f_inode)]->channel_id, channel_num);
            return channel->msg_len;
        }
        channel = channel->next;
    }

    // invalid channel id, channel does not exist or slot not opened
    printk(KERN_INFO "Failed to read from slot number %d channel number %d", iminor(file->f_inode), channel_num);
    return -EINVAL;
}

//---------------------------------------------------------------
// a processs which has already opened
// the device file attempts to write to it
// in ex. 3, message length is limited to 128 bytes
static ssize_t device_write(struct file *file,
                            const char __user *buffer,
                            size_t length,
                            loff_t *offset)
{
    ssize_t i;
    unsigned int channel_num;
    slot_channel *channel = NULL;
    if (buffer == NULL) // check if user buffer is valid
    {
        return -EINVAL;
    }
    if ((length > BUF_LEN) || (length == 0)) // check if message is too long or empty
    {
        return -EMSGSIZE;
    }
    if (file == NULL || file->private_data == NULL) // check if file is valid(no NULLPOINTEREXCEP since || and file is checked first)
    {
        return -EINVAL;
    }
    channel_num = (unsigned int)(unsigned long)(file->private_data);
    if (channel_num == 0) // check if channel set on fd
    {
        return -EINVAL;
    }
    channel = device_info.channels[iminor(file->f_inode)]; // get slot
    if (channel == NULL)                                   // check if slot was opened
    {
        return -EINVAL;
    }

    while (channel->next != NULL) // find the channel
    {
        if (channel->channel_id == channel_num)
        {
            break;
        }
        channel = channel->next;
    }
    if ((channel_num != channel->channel_id) && (channel->channel_id != 0)) // if channel doesn't exist and isn't first
    {
        channel->next = kmalloc(sizeof(slot_channel), GFP_KERNEL);
        if (channel->next == NULL)
        {
            return -ENOMEM;
        }
        channel = channel->next;
        channel->next = NULL;
    }

    if (channel->message != NULL) // free old message
    {
        kfree(channel->message);
    }
    channel->message = kmalloc(length, GFP_KERNEL);
    if (channel->message == NULL)
    {
        return -ENOMEM;
    }
    for (i = 0; i < length; i++)
    {
        get_user(channel->message[i], &buffer[i]);
    }
    channel->msg_len = length;
    channel->channel_id = channel_num;

    printk(KERN_INFO "Wrote to slot number %d channel number %d, message: %s\n", iminor(file->f_inode), channel_num, channel->message);
    printk(KERN_INFO "Slot number %d first channel num is %d", iminor(file->f_inode), device_info.channels[iminor(file->f_inode)]->channel_id);

    // return the number of input characters used
    return i;
}

//----------------------------------------------------------------
static long device_ioctl(struct file *file,
                         unsigned int ioctl_command_id,
                         long unsigned int ioctl_param) // check if channel exists, if not create it and set it to private_data
{
    // Switch according to the ioctl called
    if ((MSG_SLOT_CHANNEL != ioctl_command_id) || (ioctl_param == 0))
    {
        // wrong passed command or channel id
        return -EINVAL;
    }

    file->private_data = (void *)(unsigned long)ioctl_param; // set channel id to private data
    // if ioctl is supposed to be called only after open, check if slot was opened
    printk(KERN_INFO "Channel number set to %d\n", (unsigned int)(unsigned long)(file->private_data));
    printk(KERN_INFO "Slot number %d first channel num is %d", iminor(file->f_inode), device_info.channels[iminor(file->f_inode)]->channel_id);

    return SUCCESS;
}

//==================== DEVICE SETUP =============================

// This structure will hold the functions to be called
// when a process does something to the device we created
struct file_operations Fops = {
    .owner = THIS_MODULE,
    .read = device_read,
    .write = device_write,
    .open = device_open,
    .unlocked_ioctl = device_ioctl,
    .release = device_release,
};

//---------------------------------------------------------------
// Initialize the module - Register the character device
static int __init simple_init(void)
{
    int rc = -1, i;
    // init dev struct
    memset(&device_info, 0, sizeof(chardev_info));
    device_info.channels = kmalloc(256 * sizeof(slot_channel *), GFP_KERNEL);
    if (device_info.channels == NULL)
    {
        return -ENOMEM;
    }
    for (i = 0; i < 256; i++)
    {
        (device_info.channels)[i] = NULL;
    }

    // Register driver capabilities. Obtain major num
    rc = register_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME, &Fops);

    // Negative values signify an error
    if (rc < 0)
    {
        printk(KERN_ERR "%s registraion failed for  %d\n",
               DEVICE_FILE_NAME, MAJOR_NUM);
        return rc;
    }

    printk("Registeration is successful. ");
    printk("If you want to talk to the device driver,\n");
    printk("you have to create a device file:\n");
    printk("mknod /dev/%s c %d 0\n", DEVICE_FILE_NAME, MAJOR_NUM);
    printk("You can echo/cat to/from the device file.\n");
    printk("Dont forget to rm the device file and "
           "rmmod when you're done\n");

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
