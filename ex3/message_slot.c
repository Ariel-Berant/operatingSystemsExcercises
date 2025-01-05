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

#include <linux/kernel.h>  /* We're doing kernel work */
#include <linux/module.h>  /* Specifically, a module */
#include <linux/fs.h>      /* for register_chrdev */
#include <linux/uaccess.h> /* for get_user and put_user */
#include <linux/string.h>  /* for memset. NOTE - not string.h!*/
#include <linux/slab.h>    /* for kmalloc */
#include <linux/unistd.h>

MODULE_LICENSE("GPL");

// Our custom definitions of IOCTL operations
#include "message_slot.h"

struct chardev_info
{
    channel channels[256];
};

// used to prevent concurent access into the same device
static int dev_open_flag = 0;

static struct chardev_info device_info;

// The message the device will give when asked
static char the_message[BUF_LEN];

//================== DEVICE FUNCTIONS ===========================
static int device_open(struct inode *inode,
                       struct file *file)
{
    int minor = iminor(inode);
    if (minor < 0 || minor > 255) // check if minor is valid
    {
        errno = EINVAL;
        return -1;
    }

    if (device_info.channels[minor] == NULL) // check if channel exists, if not create it
    {
        device_info.channels[minor] = kmalloc(sizeof(channel), GFP_KERNEL);
        if (device_info.channels[minor] == NULL)
        {
            errno = ENOMEM;
            return -1;
        }
        
        device_info.channels[minor]->channel_id = -1;
        device_info.channels[minor]->msg_len = -1;
        device_info.channels[minor]->next = NULL;
        return SUCCESS;
    }
    else // channel exists, can't open
    {
        errno = EEXIST;
        return -1;
    }
    
    // ensure code is ok and (maybe) works?
}

//---------------------------------------------------------------
static int device_release(struct inode *inode,
                          struct file *file) // free even if not allocated, check if ok
{
    int minor = iminor(inode);
    if (minor < 0 || minor > 255)
    {
        errno = EINVAL;
        return -1;
    }

    if (device_info.channels[minor] != NULL)
    {
        channel *temp = device_info.channels[minor];
        while (temp != NULL) // free all channels - free current, then go to next
        {
            channel *next = temp->next;
            kfree(temp);
            temp = next;
        }
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
                           loff_t *offset) // NOT FINISHED, CONTINUE FROM HERE
{
    if ((length > BUF_LEN) || (length == 0))
    {
        errno = EMSGSIZE;
        return -1;
    }
    if ((file->private_data == NULL))
    {
        errno = EINVAL;
        return -1;
    }
    int channel_num = (int)file->private_data;
    channel *channel = device_info.channels[channel_num];
    if (channel == NULL)
    {
        errno = EWOULDBLOCK;
        return -1;
    }
    while (channel != NULL)
    {
        if (channel->channel_id == channel_num)
        {
            if (channel->msg_len == -1)
            {
                errno = EINVAL;
                return -1;
            }
            if (channel->msg_len > length)
            {
                errno = ENOSPC;
                return -1;
            }
            for (int i = 0; i < channel->msg_len; i++)
            {
                put_user(channel->message[i], &buffer[i]);
            }
            return channel->msg_len;
        }
        channel = channel->next;
    }
    

    // invalid argument error
    errno = EINVAL;
    return -1;
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
    printk("Invoking device_write(%p,%ld)\n", file, length);
    for (i = 0; i < length && i < BUF_LEN; ++i)
    {
        get_user(the_message[i], &buffer[i]);
        if (1 == encryption_flag)
            the_message[i] += 1;
    }

    // return the number of input characters used
    return i;
}

//----------------------------------------------------------------
static long device_ioctl(struct file *file,
                         unsigned int ioctl_command_id,
                         unsigned int ioctl_param) // check if channel exists, if not create it and set it to private_data
{
    // Switch according to the ioctl called
    if ((MSG_SLOT_CHANNEL != ioctl_command_id) || (ioctl_param == 0))
    {
        // wrong passed command
        errno = EINVAL;
        return -1;
    }

    file->private_data = (void *)ioctl_param;

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
    int rc = -1;
    // init dev struct
    memset(&device_info, 0, sizeof(struct chardev_info));
    for (int i = 0; i < 256; i++)
    {
        device_info.channels[i] = NULL;
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
