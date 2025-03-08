#include <linux/ctype.h>
#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/fcntl.h>
#include <asm/system.h>
#include <asm/uaccess.h>
#include <linux/string.h>

#include "encdec.h"

#define MODULE_NAME "encdec"

#define Modulo 128

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Adan_Moataz");

int 	encdec_open(struct inode *inode, struct file *filp);
int 	encdec_release(struct inode *inode, struct file *filp);
int 	encdec_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg);

ssize_t encdec_read_caesar( struct file *filp, char *buf, size_t count, loff_t *f_pos );
ssize_t encdec_write_caesar(struct file *filp, const char *buf, size_t count, loff_t *f_pos);

ssize_t encdec_read_xor( struct file *filp, char *buf, size_t count, loff_t *f_pos );
ssize_t encdec_write_xor(struct file *filp, const char *buf, size_t count, loff_t *f_pos);

char* xor_buffer;
char* ceaser_buffer;

int memory_size = 0;

MODULE_PARM(memory_size, "i");

int major = 0;

struct file_operations fops_caesar =
{
    .open 	 =	encdec_open,
    .release =	encdec_release,
    .read 	 =	encdec_read_caesar,
    .write 	 =	encdec_write_caesar,
    .llseek  =	NULL,
    .ioctl 	 =	encdec_ioctl,
    .owner 	 =	THIS_MODULE
};

struct file_operations fops_xor =
{
    .open 	 =	encdec_open,
    .release =	encdec_release,
    .read 	 =	encdec_read_xor,
    .write 	 =	encdec_write_xor,
    .llseek  =	NULL,
    .ioctl 	 =	encdec_ioctl,
    .owner 	 =	THIS_MODULE
};

// Implemetation suggestion:
// -------------------------
// Use this structure as your file-object's private data structure
typedef struct
{
    unsigned char key;
    int read_state;
} encdec_private_date;

int init_module(void)
{
    major = register_chrdev(major, MODULE_NAME, &fops_caesar);
    if(major < 0)
    {
        return major;
    }

    /*memory allocation for the devices*/
    ceaser_buffer = (char *)kmalloc(memory_size * sizeof(char), GFP_KERNEL);
    xor_buffer = (char *)kmalloc(memory_size * sizeof(char), GFP_KERNEL);

    /*Error handling for the two device buffers.*/
    if(ceaser_buffer == NULL || xor_buffer == NULL)
    {
        printk("buffer allocation failed");
        return -1;
    }

    return 0;
}

void cleanup_module(void)
{
    // Implemetation suggestion:

    /*1. Unregister the device-driver*/
    unregister_chrdev(major, MODULE_NAME);

    /* 2. Free the allocated device buffers using kfree*/
    /*free Caesar and Xor buffer*/
    kfree(ceaser_buffer);
    kfree(xor_buffer);

    return;
}

int encdec_open(struct inode *inode, struct file *filp)
{
    /*Implemetation suggestion:
     -------------------------
     1. Set 'filp->f_op' to the correct file-operations structure (use the minor value to determine which)
     2. Allocate memory for 'filp->private_data' as needed (using kmalloc)*/

    switch (MINOR(inode->i_rdev))
    {
    case 0:
    {
        filp->f_op = &(fops_caesar);
        break;
    }
    case 1:
    {
        filp->f_op = &(fops_xor);
        break;
    }
    default:
    {
        return -ENODEV;
        break;
    }
    }

    /*Allocate memory using kmalloc*/
    encdec_private_date* curr_private_data = kmalloc(sizeof(encdec_private_date), GFP_KERNEL);
    curr_private_data->read_state = ENCDEC_READ_STATE_DECRYPT;
    curr_private_data->key = 0;
    filp->private_data = curr_private_data;

    return 0;
}

int encdec_release(struct inode *inode, struct file *filp)
{
    /* Implemetation suggestion:
     -------------------------*/

    if(!inode || !filp)
        return -EINVAL;

    /*1. Free the allocated memory for 'filp->private_data' (using kfree)*/
    kfree(filp -> private_data);

    return 0;
}

int encdec_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
    /* Implemetation suggestion:
     -------------------------
     1. Update the relevant fields in 'filp->private_data' according to the values of 'cmd' and 'arg'*/

    if(cmd == ENCDEC_CMD_CHANGE_KEY)
    {
        ((encdec_private_date*)(filp->private_data))->key = (unsigned char)arg;
        return 0;
    }
    if(cmd == ENCDEC_CMD_SET_READ_STATE)
    {
        ((encdec_private_date*)(filp->private_data))->read_state = (int)arg;
        return 0;
    }
    if(cmd == ENCDEC_CMD_ZERO)
    {
        if (MINOR(inode->i_rdev) == 0)
            memset(ceaser_buffer, 0, memory_size);

        if (MINOR(inode->i_rdev) == 1)
            memset(xor_buffer, 0, memory_size);
        return 0;
    }
    else
        return -ENODEV;
}

/* Add implementations for:
 ------------------------*/
/* 1. ssize_t encdec_read_caesar( struct file *filp, char *buf, size_t count, loff_t *f_pos );*/

ssize_t encdec_read_caesar( struct file *filp, char *buf, size_t count, loff_t *f_pos )
{
    int i;

    encdec_private_date* curr_private_data = filp->private_data;

    if (*f_pos == memory_size)
        return -EINVAL;

    if(count >= memory_size - *f_pos)
        count = memory_size - *f_pos;


    if (curr_private_data->read_state == ENCDEC_READ_STATE_DECRYPT)
    {
        for (i = 0; i < count; i++)
        {
            ceaser_buffer[(*f_pos) + i] = ((ceaser_buffer[(*f_pos) + i] - (curr_private_data->key)) + Modulo) % Modulo;
        }

        if(copy_to_user(buf, ceaser_buffer + (*(f_pos)), count) != 0)
            return -EFAULT;

        for (i = 0; i < count; i++)
        {
            ceaser_buffer[(*f_pos) + i] = ((ceaser_buffer[(*f_pos) + i] + (curr_private_data->key)) % Modulo);
        }
    }
    else if (curr_private_data->read_state == ENCDEC_READ_STATE_RAW)
    {
        if(copy_to_user(buf, ceaser_buffer + (*(f_pos)), count) != 0 )
            return -EFAULT;
    }

    *(f_pos) = *(f_pos) + count;

    return count;
}

/* 2. ssize_t encdec_write_caesar(struct file *filp, const char *buf, size_t count, loff_t *f_pos);*/

ssize_t encdec_write_caesar(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
    int i;

    if(filp == NULL || buf == NULL)
        return -EINVAL;

    if (*f_pos == memory_size)
        return -ENOSPC;

    if (count >= memory_size - *f_pos)
        count = memory_size - *f_pos;

    if((copy_from_user(ceaser_buffer + *f_pos, buf, count)) !=0 )
        return -EFAULT;

    for (i = *f_pos; i < (*f_pos + count); i++)
    {
        ceaser_buffer[i] = (ceaser_buffer[i] + ((encdec_private_date*)(filp->private_data))->key) % Modulo;
    }

    *(f_pos) = *(f_pos) + count;

    return count;
}

/* 3. ssize_t encdec_read_xor( struct file *filp, char *buf, size_t count, loff_t *f_pos );*/

ssize_t encdec_read_xor( struct file *filp, char *buf, size_t count, loff_t *f_pos )
{
    int i, tmp;
    encdec_private_date* curr_private_data = filp->private_data;

    if(count >= memory_size - *f_pos)
        count = memory_size - *f_pos;

    if (*f_pos == memory_size)
        return -EINVAL;

    if (curr_private_data->read_state == ENCDEC_READ_STATE_RAW)
    {
        if(copy_to_user(buf, xor_buffer + (*(f_pos)), count) != 0)
            return -EFAULT;
    }
    else
    {
        tmp = curr_private_data->key;

        for (i = 0; i < count; i++)
        {
            xor_buffer[(*f_pos) + i] = xor_buffer[(*f_pos) + i] ^ tmp;
        }

        if(copy_to_user(buf, xor_buffer + (*(f_pos)), count)  != 0 )
            return -EFAULT;

        for (i = 0; i < count; i++)
        {
            xor_buffer[(*f_pos) + i] = xor_buffer[(*f_pos) + i] ^ tmp;
        }
    }

    *(f_pos) = *(f_pos) + count;

    return count;
}

/* 4. ssize_t encdec_write_xor(struct file *filp, const char *buf, size_t count, loff_t *f_pos);*/

ssize_t encdec_write_xor(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
    int i, tmp;

    encdec_private_date* curr_private_data = filp->private_data;

    if(filp == NULL || buf == NULL)
        return -EINVAL;

    if(*f_pos == memory_size)
        return -ENOSPC;

    if(count >= memory_size - *f_pos)
        count = memory_size - *f_pos;

    if(copy_from_user(xor_buffer + *f_pos, buf, count) != 0)
        return -EFAULT;

    tmp = curr_private_data->key;

    for (i = *f_pos; i < (*f_pos + count); i++)
    {
        xor_buffer[i] = xor_buffer[i] ^ tmp;
    }

    *(f_pos) = *(f_pos) + count;

    return count;

}





