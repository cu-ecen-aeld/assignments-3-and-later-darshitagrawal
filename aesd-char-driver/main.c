/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes
 * @date 2019-10-22
 * @copyright Copyright (c) 2019
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h> // file_operations
#include "aesdchar.h"
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Darshit"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
	struct aesd_dev *dev;
	PDEBUG("open");
	dev = container_of(inode->i_cdev, struct aesd_dev, cdev);
	filp->private_data = dev;
	/**
	 * TODO: handle open
	 */
	return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
	PDEBUG("release");
	/**
	 * TODO: handle release
	 */
	return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
	ssize_t retval = 0; 
	struct aesd_dev *dev = filp->private_data;
	size_t bytes_read = 0;
	size_t entry_offset = 0;
	size_t remaining_count = 0;
	int result_copy;
	struct aesd_buffer_entry *read_entry;

	PDEBUG("read %zu bytes with offset %lld",count,*f_pos);

	if(mutex_lock_interruptible(&dev->lock))
	{
		retval = -ERESTARTSYS;
		return retval; 
	}

	read_entry = aesd_circular_buffer_find_entry_offset_for_fpos(&dev->buffer_offset, *f_pos, &entry_offset);
	if(read_entry == NULL) 
	{
		retval = 0;
		goto out;
	}

	remaining_count = read_entry->size - entry_offset;
	
	if(remaining_count > count)
	{
		bytes_read = count;
	}
	else 
	{
		bytes_read = remaining_count;
	}

	result_copy = copy_to_user(buf, read_entry->buffptr + entry_offset, bytes_read);

	if(result_copy)
	{
		retval = -EFAULT;
		goto out;
	}

	*f_pos += bytes_read;
	retval = bytes_read;

	out:
		mutex_unlock(&dev->lock);
		return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
	ssize_t retval = -ENOMEM;
	PDEBUG("write %zu bytes with offset %lld",count,*f_pos);
	/**
	 * TODO: handle write
	 */
	struct aesd_dev* dev = filp->private_data;
	size_t result_copy;
	const char* new_entry = NULL;

	if(mutex_lock_interruptible(&dev->lock))
	{
		retval = -ERESTARTSYS;
		return retval;
	}
	if(dev->circular_buffer.size == 0)
	{
		dev->circular_buffer.buffptr = kzalloc(count, GFP_KERNEL);
	}
	else 
	{
		dev->circular_buffer.buffptr = krealloc(dev->circular_buffer.buffptr, dev->circular_buffer.size + count, GFP_KERNEL);
	}

	if(dev->circular_buffer.buffptr == NULL)
	{
		retval = -ENOMEM;
		goto out;
	} 

	retval = count;

	result_copy = copy_from_user((void*)(&dev->circular_buffer.buffptr[dev->circular_buffer.size]),
											buf, count);

	if(result_copy)
	{
		retval -= result_copy;
	}

	dev->circular_buffer.size += retval;  

	if(strchr((char*)(dev->circular_buffer.buffptr), '\n'))
	{
		new_entry = aesd_circular_buffer_add_entry(&dev->buffer_offset, &dev->circular_buffer);
		if(new_entry)
		{
			kfree(new_entry);
		}
		dev->circular_buffer.buffptr = NULL;
		dev->circular_buffer.size = 0; 
	}

	out: 
		mutex_unlock(&dev->lock);
		return retval;
		
	return retval;
}
struct file_operations aesd_fops = {
	.owner =    THIS_MODULE,
	.read =     aesd_read,
	.write =    aesd_write,
	.open =     aesd_open,
	.release =  aesd_release,
};

static int aesd_setup_cdev(struct aesd_dev *dev)
{
	int err, devno = MKDEV(aesd_major, aesd_minor);

	cdev_init(&dev->cdev, &aesd_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &aesd_fops;
	err = cdev_add (&dev->cdev, devno, 1);
	if (err) {
		printk(KERN_ERR "Error %d adding aesd cdev", err);
	}
	return err;
}



int aesd_init_module(void)
{
	dev_t dev = 0;
	int result;
	result = alloc_chrdev_region(&dev, aesd_minor, 1,
			"aesdchar");
	aesd_major = MAJOR(dev);
	if (result < 0) {
		printk(KERN_WARNING "Can't get major %d\n", aesd_major);
		return result;
	}
	memset(&aesd_device,0,sizeof(struct aesd_dev));

	//aesd_circular_buffer_init(&(aesd_device.cir_buffer));
	mutex_init(&(aesd_device.lock));
	/**
	 * TODO: initialize the AESD specific portion of the device
	 */

	result = aesd_setup_cdev(&aesd_device);

	if( result ) {
		unregister_chrdev_region(dev, 1);
	}
	return result;

}

void aesd_cleanup_module(void)
{
	dev_t devno = MKDEV(aesd_major, aesd_minor);

	cdev_del(&aesd_device.cdev);

	/**
	 * TODO: cleanup AESD specific poritions here as necessary
	 */
	aesd_circular_buffer_free(&aesd_device.buffer_offset);
	unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
