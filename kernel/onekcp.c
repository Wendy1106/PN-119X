#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/gpio.h>
#include <asm/gpio.h>
#include <linux/of_gpio.h>
#include <linux/of.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/kmod.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/syscalls.h>
#include <linux/unistd.h>

MODULE_AUTHOR("Danza");
MODULE_LICENSE("Dual BSD/GPL");

static struct workqueue_struct *onekcp_workqueue; 
static struct work_struct onekcp_work;
//static int do_onekcp(void *)
//char pathx[ ] = "/bin/touch";
char pathx[ ] = "/sbin/onekcp";
char* argvx[ ] = {pathx,NULL};
//char* argvx[ ] = {pathx,"/tmp/onekcp",NULL};
char* envpx[ ] = {"HOME=/root","PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin",NULL};

struct onekcp{
	unsigned int tag ;
	unsigned int gpio2irq;
	unsigned int FUNC_sw;
	unsigned int INFO_led;
	unsigned int infoled_value;
	struct timer_list onekcp_timer;
	struct timer_list ledblink_timer;
}onekcp;

static int do_onekcp(void *arg)
{
	int ret;
	//gpio_set_value(onekcp.INFO_led,1);
	onekcp.tag = 1;
	mdelay(10);
	mod_timer(&onekcp.ledblink_timer,jiffies + msecs_to_jiffies(5));
	mdelay(10);
	ret = call_usermodehelper(pathx,argvx,envpx,UMH_WAIT_PROC);
	mdelay(10);
	onekcp.tag = 0;
	return 0;
}

static irqreturn_t onekcp_irq(int irq, void *dev_id)
{
	int flag;
	flag = gpio_get_value(onekcp.FUNC_sw);
	if(flag == 0)
	{
		mod_timer(&onekcp.onekcp_timer,jiffies + msecs_to_jiffies(500));
	}
	return IRQ_HANDLED;
}

static void onekcp_timer_function(unsigned long data)
{
	int flag;
	flag = gpio_get_value(onekcp.FUNC_sw);
	if(flag == 0)
	{
		printk(KERN_ALERT"onekcp: prepare to do copy_work!\n");
		schedule_work(&onekcp_work);
	}
}


static void ledblink_timer_function(unsigned long data)
{
	if(onekcp.tag == 1)
	{
		onekcp.infoled_value = !(onekcp.infoled_value);
		gpio_set_value(onekcp.INFO_led,onekcp.infoled_value);
		mod_timer(&onekcp.ledblink_timer,jiffies + msecs_to_jiffies(50));
	}
	else
	{
		//turn off infoled lights for logic correction
		gpio_set_value(onekcp.INFO_led,0);
	}
}

static int __init onekcp_init(void)
{
	int err;
	struct device_node *p_node = NULL;
	printk("onekcp: init success!\n");
	onekcp_workqueue = create_singlethread_workqueue("onekcp");
	INIT_WORK(&onekcp_work,do_onekcp);
	p_node = of_find_node_by_path("/onekcp");
	if(p_node == NULL)
	{
		printk(KERN_ALERT"onekcp: Current is in rescue mode cause of find no FDT node!\n");
		return -1;
	}
	if(p_node != NULL)
	{
		onekcp.FUNC_sw = of_get_named_gpio(p_node,"FUNC_sw",0);
		onekcp.INFO_led = of_get_named_gpio(p_node,"INFO_led",0);
	}
	printk("onekcp:onekcp.FUNC_sw is gpio:%d\n",onekcp.FUNC_sw);
	printk("onekcp.onekcp.INFO_led is gpio:%d\n",onekcp.INFO_led);

	if(gpio_request(onekcp.FUNC_sw,NULL) || gpio_request(onekcp.INFO_led,NULL))
	{
		gpio_free(onekcp.FUNC_sw);
		gpio_free(onekcp.INFO_led);
		printk(KERN_ALERT"onekcp:GPIO request fail\n");
		return -2;
	}
	gpio_direction_input(onekcp.FUNC_sw);
	gpio_direction_output(onekcp.INFO_led,0);

	init_timer(&onekcp.onekcp_timer);
	onekcp.onekcp_timer.function = onekcp_timer_function;
	onekcp.onekcp_timer.expires = jiffies + msecs_to_jiffies(200);
	add_timer(&onekcp.onekcp_timer);

	onekcp.tag = 0;

	init_timer(&onekcp.ledblink_timer);
	onekcp.ledblink_timer.function = ledblink_timer_function;
	onekcp.ledblink_timer.expires = jiffies + msecs_to_jiffies(50);
	add_timer(&onekcp.ledblink_timer);

	onekcp.gpio2irq = gpio_to_irq(onekcp.FUNC_sw);
	err = request_irq(onekcp.gpio2irq,onekcp_irq,IRQF_TRIGGER_FALLING,"onekcp",NULL);
	printk("onekcp:request_irq results: %d\n",err);
	return 0;

}
static void __exit onekcp_exit(void)
{
	printk("onekcp:BYEBYE\n");
	free_irq(onekcp.gpio2irq,NULL);
	gpio_free(onekcp.FUNC_sw);
	gpio_free(onekcp.INFO_led);
	destroy_workqueue(onekcp_workqueue);
	del_timer(&onekcp.onekcp_timer);
}

module_init(onekcp_init);
module_exit(onekcp_exit);



























