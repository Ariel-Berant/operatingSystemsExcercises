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
        errno = EINVAL;
        return -1;
    }

    if (device_info.channels[minor] != NULL)
    {
        channel *temp = device_info.channels[minor];
        while (temp != NULL) // free all channels - free current, then go to next
        {
            channel *next = temp->next;
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
    unsigned int channel_num = (unsigned int)file->private_data;
    if (channel_num == 0) // check if channel set on fd
    {
        errno = EINVAL;
        return -1;
    }
    channel *channel = device_info.channels[iminor(file->f_inode)]; // get slot
    while (channel != NULL)
    {
        if (channel->channel_id == channel_num)
        {
            if (channel->msg_len > length) // check if message is too long
            {
                errno = ENOSPC;
                return -1;
            }
            if (channel->message == NULL) // check if a message exists on the channel
            {
                errno = EWOULDBLOCK;
                return -1;
            }
            if (buffer == NULL) // check if buffer is valid
            {
                errno = EINVAL;
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
    

    // invalid channel id, channel does not exist
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
    if (buffer == NULL) // check if user buffer is valid
    {
        errno = EINVAL;
        return -1;
    }
    if ((length > BUF_LEN) || (length == 0)) // check if message is too long or empty
    {
        errno = EMSGSIZE;
        return -1;
    }
if (file == NULL || file->private_data == NULL) // check if file is valid(no NULLPOINTEREXCEP since || and file is checked first)
    {
        errno = EINVAL;
        return -1;
    }
    unsigned int channel_num = (unsigned int)file->private_data;
    if (channel_num == 0) // check if channel set on fd
    {
        errno = EINVAL;
        return -1;
    }
    channel *channel = device_info.channels[iminor(file->f_inode)]; // get slot
    if (channel == NULL) // check if slot was opened
    {
        errno = EINVAL;
        return -1;
    }
    
    while (channel->next != NULL) //find the channel
    {
        if (channel->channel_id == channel_num)
        {
            break;
        }
        channel = channel->next;
    }
    if ((channel_num != channel->channel_id) && (channel->channel_id != 0)) // if channel doesn't exist and isn't first
    {
        channel->next = kmalloc(sizeof(channel), GFP_KERNEL);
        if (channel->next == NULL)
        {
            errno = ENOMEM;
            return -1;
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
        errno = ENOMEM;
        return -1;
    }
    for (i = 0; i < length; i++)
    {
        get_user(channel->message[i], &buffer[i]);
    }
    channel->msg_len = length;
    channel->channel_id = channel_num;

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
        // wrong passed command or channel id
        errno = EINVAL;
        return -1;
    }

    file->private_data = (void *)ioctl_param; // set channel id to private data
    // if ioctl is supposed to be called only after open, check if slot was opened

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
