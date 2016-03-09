#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <asm/uaccess.h>

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Danza");

#define DS1820_CMD_SKIP_ROM	(0xCC)
#define DS1820_CMD_CONVERT	(0x44)
#define DS1820_CMD_READSCR	(0xBE)

#define DS1820_DEV_NAME "ds18b20"

struct ds1820_device
{
	struct mutex res_mutex;
	struct cdev cdev;
};
static int ds1820_major = 0;
static struct ds1820_device *ds1820_devp;
static struct class *my_class;

#define ds1820_existed() ds1820_reset()
static unsigned int ds1820_reset(void)
{
	unsigned int ret;
	gpio_direction_output(4,0);
	udelay(600);	/* hold for 480~960us */
	gpio_set_value(4,1);	/* release dq */
	gpio_direction_input(4);
	udelay(60);	/* wait for 60us, ds1820 will hold dq low for 240us */
	ret = ~gpio_get_value(4);/* ds1820 pulldown dq */
	udelay(240);
	return ret;
}

static void ds1820_write_byte(unsigned char byte)
{
	unsigned int i;
	gpio_direction_output(4,1);
	for (i = 8; i != 0; --i) {
		gpio_set_value(4,0);
		udelay(10); 	/* delay 10us, smaller than 15us */
		if (byte & 0x01)
			gpio_set_value(4,1);

		udelay(50);	/* delay (10 + 50 >= 60us) */
		gpio_set_value(4,1);	/* set dq high for next write slot */
		udelay(3);	/* delay a while */
		byte >>= 1;
	}
}

static unsigned char ds1820_read_byte(void)
{
	unsigned int i;
	unsigned char ret = 0x00;
	for (i = 8; i != 0; --i) {
		ret >>= 1;
		gpio_direction_output(4,0);
		udelay(2);	/* falling edge of dq > 1us */
		gpio_direction_input(4);
		udelay(4);	/* sample window < 15us */
		if (gpio_get_value(4))
			ret |= 0x80;
		udelay(60);	/* delay (2 + 4 + 60 > 60us) */
	}
	return ret;
}

static int ds1820_open(struct inode *inode, struct file *filp)
{
	struct ds1820_device *dev;
	int retval;
	dev = container_of(inode->i_cdev, struct ds1820_device, cdev);
	filp->private_data = dev;
	mutex_lock_interruptible(&dev->res_mutex);
	retval = ds1820_existed() ? 0: -ENODEV;
	mutex_unlock(&dev->res_mutex);
	return retval;
}

static int ds1820_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static ssize_t ds1820_read(struct file *filp, char *buf, size_t count,loff_t *pos)
{
	struct ds1820_device *dev;
	unsigned char tmp[2];
	unsigned long err;

	dev = filp->private_data;
	mutex_lock_interruptible(&dev->res_mutex);
	/* send temperature convert command */
	ds1820_reset();
	ds1820_write_byte(DS1820_CMD_SKIP_ROM);
	ds1820_write_byte(DS1820_CMD_CONVERT);
	udelay(3);/* wait a while */

	/* send read scratchpad command */
	ds1820_reset();
	ds1820_write_byte(DS1820_CMD_SKIP_ROM);
	ds1820_write_byte(DS1820_CMD_READSCR);

	/* read temperature data */
	tmp[0] = ds1820_read_byte();
	tmp[1] = ds1820_read_byte();
	ds1820_reset();	/* terminate the data transfer, we only need 2 bytes */
	err = copy_to_user(buf, tmp, sizeof(tmp));
	mutex_unlock(&dev->res_mutex);
	return err ? -EFAULT: min(sizeof(tmp), count);
}

static struct file_operations ds1820_fops =
{
	.owner 	= THIS_MODULE,
	.open	= ds1820_open,
	.release= ds1820_release,
	.read	= ds1820_read
};

static int __init ds1820_init(void)
{
	int result;
	dev_t devno = 0;
	
	struct device_node *p_node = NULL;
	printk("18B20: init success!\n");
	p_node = of_find_node_by_path("/ds18b20");

	if(p_node == NULL)
	{
		printk(KERN_ALERT"18B20: current is in rescue mode cause find no FDT node!\n");
		return -1;
	}
	printk("18B20: success find FDT Node!\n");
	if(gpio_request(4,NULL))
	{
		gpio_free(4);
		printk(KERN_ALERT"18B20: gpio request failed!\n");
		return -2;
	}
	//register cdev
	if (ds1820_major) {
		devno = MKDEV(ds1820_major, 0);
		result = register_chrdev_region(devno, 1, DS1820_DEV_NAME);
	} else {
		result = alloc_chrdev_region(&devno, 0, 1, DS1820_DEV_NAME);
		ds1820_major = MAJOR(devno);
	}

	if (result < 0) {
		printk(KERN_ERR "ds1820: init error!\n");
		return result;
	}

	ds1820_devp = kzalloc(sizeof(struct ds1820_device), GFP_KERNEL);
	if (!ds1820_devp) {
		result = -ENOMEM;
		goto fail;
	}

	mutex_init(&ds1820_devp->res_mutex);
	/* setup cdev */
	cdev_init(&ds1820_devp->cdev, &ds1820_fops);
	ds1820_devp->cdev.owner = THIS_MODULE;
	ds1820_devp->cdev.ops = &ds1820_fops;
	if (cdev_add(&ds1820_devp->cdev, devno, 1))
		printk(KERN_NOTICE "Error adding ds1820 cdev!\n");

	/* setup device node */
	my_class = class_create(THIS_MODULE, "ds1820_class");
	device_create(my_class, NULL, MKDEV(ds1820_major, 0), \
				NULL, DS1820_DEV_NAME);
	printk(KERN_NOTICE "18B20: All is ready! try to run app to get temp value.\n");
	return 0;
fail:
	unregister_chrdev_region(devno, 1);
	return result;
}

static void __exit ds1820_exit(void)
{
	dev_t devno = MKDEV(ds1820_major, 0);

	if (ds1820_devp)
		cdev_del(&ds1820_devp->cdev);

	kfree(ds1820_devp);
	device_destroy(my_class, devno);
	class_destroy(my_class);

	unregister_chrdev_region(devno, 1);
	printk(KERN_NOTICE "bye ds1820!\n");
}

module_init(ds1820_init);
module_exit(ds1820_exit);