#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/jump_label.h>
#include <linux/string.h>

#define DEVICE_NAME "yc_cdrv"

static dev_t dev_num;
static struct cdev *driver_object;
static struct class *class;
//static bool value;
static DEFINE_STATIC_KEY_FALSE(fkey);

static int driver_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int driver_close(struct inode *inode, struct file *file)
{
	return 0;
}

static ssize_t driver_write(struct file *file, const char *buff, size_t len,
			    loff_t *offp)
{
	char command[10];
	if (len > 10)
		pr_err("command exceeded 10 char\n");

	if(copy_from_user(command, buff, len))
		return -EFAULT;

	if (strncmp(command, "enable", strlen("enable")) == 0)
		static_branch_enable(&fkey);
	else if (strncmp(command, "disable", strlen("disable")) == 0)
		static_branch_disable(&fkey);
	else if (strncmp(command, "status", strlen("status")) == 0)
		pr_info("key status: %s\n",
			static_key_enabled(&fkey) ? "enabled" : "disabled");
	else if (strncmp(command, "run", strlen("run")) == 0) {
		pr_info("do code 1\n");
		if (static_branch_unlikely(&fkey))
			pr_info("do unlikely code\n");
		pr_info("do code 2\n");
	}
	else
		return -EINVAL;

	return len;
}

static struct file_operations fops = { .owner = THIS_MODULE,
	.write = driver_write,
	.open = driver_open,
	.release = driver_close };

static int __init yc_driver_init(void)
{
	/*Allocating Major number*/
	if ((alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME)) < 0) {
		printk(KERN_INFO "Cannot allocate major number\n");
		return -1;
	}
	printk(KERN_INFO "Major = %d Minor = %d\n", MAJOR(dev_num),
	       MINOR(dev_num));

	/*Creating cdev structure*/
	driver_object = cdev_alloc();
	if (driver_object == NULL) {
		printk(KERN_INFO "Cannot allocate memory to cdev\n");
		goto free_device_number;
	}

	/*Defining File operations*/
	driver_object->ops = &fops;
	driver_object->owner = THIS_MODULE;

	/*Adding character device to the system*/
	if ((cdev_add(driver_object, dev_num, 1)) < 0) {
		printk(KERN_INFO "[YC_CDRV] Cannot add the device to system\n");
		goto free_cdev;
	}

	/*Creating struct class*/
	if ((class = class_create(THIS_MODULE, DEVICE_NAME)) == NULL) {
		printk(KERN_INFO "[YC_CDRV] Cannot create the struct class\n");
		goto free_cdev;
	}

	/*Creating device*/
	if ((device_create(class, NULL, dev_num, NULL, DEVICE_NAME)) == NULL) {
		printk(KERN_INFO "[YC_CDRV] Cannot create the Device\n");
		goto free_class;
	}

	printk(KERN_INFO "[YC_CDRV] Device Driver Insert...Done\n");

	printk(KERN_INFO "[YC_CDRV] Device Driver: fkey is %s\n", static_key_enabled(&fkey) ? "enabled" : "disabled");

	return 0;

free_class:
	class_destroy(class);
free_cdev:
	kobject_put(&driver_object->kobj);
free_device_number:
	unregister_chrdev_region(dev_num, 1);
	return -1;
}

void __exit yc_driver_exit(void)
{
	device_destroy(class, dev_num);
	class_destroy(class);
	cdev_del(driver_object);
	unregister_chrdev_region(dev_num, 1);
	printk(KERN_INFO "Device Driver Remove...Done\n");
}

module_init(yc_driver_init);
module_exit(yc_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("YC");
MODULE_DESCRIPTION("A pseudo character for test static key");
