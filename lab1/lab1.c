#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>


static int dev_open(struct inode *i, struct file *f);
static int dev_close(struct inode *i, struct file *f);
static ssize_t dev_read(struct file *f, char __user *ubuf, size_t len, loff_t *ppos);
static ssize_t dev_write(struct file *f, const char __user *ubuf, size_t len, loff_t *ppos);

static ssize_t proc_read(struct file *f, char __user *ubuf, size_t len, loff_t *ppos);
static ssize_t proc_write(struct file *f, const char __user *ubuf, size_t len, loff_t *ppos);

#define DRIVER_NAME "lab_dev"
#define DRIVER_CLASS "lab_class"
#define DEV_NAME "lab"
#define PROC_FILENAME "VAR4"
#define BUF_SIZE 200
#define MAX_SIZE 100

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Behruz Mansurov and Amir Kadyrov");
MODULE_DESCRIPTION("A simple character device [VAR4]");

static dev_t first; // Global variable for the first device number
static struct cdev c_dev; // Global variable for the character device structure
static struct class *cl; // Global variable for the device class
static struct proc_dir_entry *entry;
static uint16_t seq_counter = 0;
static uint32_t sequence[100];


static int dev_open(struct inode *i, struct file *f) {
	printk(KERN_INFO "[VAR4]: open()\n");
	return 0;
}

static int dev_close(struct inode *i, struct file *f) {
	printk(KERN_INFO "[VAR4]: close()\n");
	return 0;
}

static void auxiliary_read(char *buffer, uint32_t * const length) {
	char *buffer_ptr = buffer;
	uint32_t i;
	for(i = 0; i < seq_counter; i ++) {
		buffer_ptr += sprintf(buffer_ptr,"%d ", sequence[i]);
	}
	*(buffer_ptr++) = "\n";
	*(buffer_ptr++) = "\0";
	*length = buffer_ptr - buffer; 
}

static ssize_t dev_read(struct file *f, char __user *ubuf, size_t len, loff_t *ppos) {
	printk(KERN_INFO "[VAR4]: read() \n");
	char buf[BUF_SIZE * sizeof(uint32_t)];
	uint32_t length;
	auxiliary_read(buf, &length);

	if(*ppos > 0 || len < length) {
		printk(KERN_INFO "[VAR4]: ERROR during read device: invalid offset");
		return 0;
	}

	if(copy_to_user(ubuf, buf, length) != 0) {
		printk(KERN_INFO "[VAR4]: ERROR during read device: failed cope to user bufffer");
		return -1;
	} 
	*ppos = length;
	return length;	
}

static ssize_t proc_read(struct file *f, char __user *ubuf, size_t len, loff_t *ppos) {
	printk(KERN_INFO "[VAR4]: proc_read() \n");
	char buf[BUF_SIZE * sizeof(uint32_t)];
	uint32_t length;
	auxiliary_read(buf, &length);

	if(*ppos > 0 || len < length) {
		printk(KERN_INFO "[VAR4]: ERROR during read proc: invalid offset");
		return 0;
	}
	
	if(copy_to_user(ubuf, buf, length) != 0) {
		printk(KERN_INFO "[VAR4]: ERROR during read proc: failed copy to user buffer");
		return -1;
	}
	*ppos += length;
	return length;
}

static ssize_t dev_write(struct file *f, const char __user *ubuf, size_t len, loff_t *ppos) {
	printk(KERN_INFO "[VAR4]: write()\n");
	char buf[BUF_SIZE];

	uint16_t space_counter = 0;
	
	if(*ppos > 0 || len > BUF_SIZE) {
		return 0;
	}
	if(copy_from_user(buf, ubuf, len) != 0) {
		return -1;
	}

	uint32_t i;
	for(i = 0; i < len; i++) {
		if(buf[i] == ' ') {
			space_counter++;
		}
	}

	if(seq_counter >= MAX_SIZE) {
		return -1;
	}

	sequence[seq_counter++] = space_counter;
	size_t length;
	length = strlen(buf);
	*ppos += length;
	return length;
}

static ssize_t proc_write(struct file *f, const char __user *ubuf, size_t len, loff_t *ppos) {
	return 0;
}

static struct file_operations dev_ops = {
	.owner = THIS_MODULE,
	.open = dev_open,
	.release = dev_close,
	.read = dev_read,
	.write = dev_write
};

static struct file_operations f_ops = {
	.owner = THIS_MODULE,
	.read = proc_read,
	.write = proc_write
};

// Constructor 
static int __init chr_driver_init(void) { 
	if(alloc_chrdev_region(&first, 0, 1, DRIVER_NAME) < 0) {
		return -1;
	}
	
	if((cl = class_create(THIS_MODULE, DRIVER_CLASS)) == NULL) {
		unregister_chrdev_region(first, 1);
		return -1;
	}
	
	if(device_create(cl, NULL, first, NULL, DEV_NAME) == NULL) {
		class_destroy(cl);
		unregister_chrdev_region(first, 1);
		return -1;
	}

	cdev_init(&c_dev, &dev_ops);
	if(cdev_add(&c_dev, first, 1) == -1) {
		device_destroy(cl,first);
		class_destroy(cl);
		unregister_chrdev_region(first, 1);
		return -1;
	}

	if((entry = proc_create(PROC_FILENAME, 0777, NULL, &f_ops)) == NULL) {
		cdev_del(&c_dev);
		device_destroy(cl,first);
		class_destroy(cl);
		unregister_chrdev_region(first, 1);
	}
	printk(KERN_INFO "[VAR4]: registered");
	return 0;
}

// Destructor
static void __exit chr_driver_exit(void) {
	cdev_del(&c_dev);
	device_destroy(cl, first);
	class_destroy(cl);
	unregister_chrdev_region(first, 1);
	proc_remove(entry);
	printk(KERN_INFO "[VAR4]: unregistered");
}

module_init(chr_driver_init);
module_exit(chr_driver_exit);