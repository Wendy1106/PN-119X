#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <asm/gpio.h>
#include <linux/of_gpio.h>
#include <linux/of.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/slab.h>

MODULE_AUTHOR("Danza");
MODULE_LICENSE("Dual BSD/GPL");

struct update_fw
{
	int INFO_led;
	int ERR_led;
	int USB_led;
	int PWR_led;
	int EXT_USB3_pwr;
	int led_value;
	struct timer_list update_fw_timer;
}update_fw;

static void timer_function(unsigned long data)
{
	update_fw.led_value = !(update_fw.led_value);
	gpio_set_value(update_fw.INFO_led,update_fw.led_value);
	mod_timer(&update_fw.update_fw_timer, jiffies + msecs_to_jiffies(200));
}

static int __init update_fw_init(void)
{
	struct device_node *p_node = NULL;
	update_fw.led_value = 0;
	printk("init update_fw led blink module\n");
	p_node = of_find_node_by_path("/update_fw");   //遍历DTB 以path：/updat_fw 的名字去寻找
	if ( p_node == NULL) {
		printk("update_fw not  in rescue mode cause of find no FDT node\n");
		return -1;
		//根据p_node的返回值去判断是rescue mode 还是normal启动；
	}
	printk("update_fw find FDT node success!\n");
	if (p_node != NULL) {
			update_fw.INFO_led=  of_get_named_gpio(p_node,"INFO_led",0);
			update_fw.EXT_USB3_pwr = of_get_named_gpio(p_node,"EXT_USB3_pwr",0);
			update_fw.USB_led = of_get_named_gpio(p_node,"USB_led",0);
			update_fw.PWR_led = of_get_named_gpio(p_node,"PWR_led",0);
		}
	//for debug purpose
	printk("update_fw.INFO_led: gpio:%d\n",update_fw.INFO_led);
	printk("update_fw.EXT_USB3_pwr: gpio:%d\n",update_fw.EXT_USB3_pwr);
	printk("update_fw.USB_led: gpio:%d\n",update_fw.USB_led);
	printk("update_fw.PWR_led: gpio:%d\n",update_fw.PWR_led);
	//gpio request for INFO_led&EXT_USB3_pwr
	gpio_request(update_fw.INFO_led,NULL);
	gpio_request(update_fw.EXT_USB3_pwr,NULL);
	gpio_request(update_fw.USB_led,NULL);
	gpio_request(update_fw.PWR_led,NULL);
	gpio_direction_output(update_fw.INFO_led,update_fw.led_value);
	//open PWR LED 
	gpio_direction_output(update_fw.PWR_led,1);
	//open EXT_USB3_pwr for install image purpose
	gpio_direction_output(update_fw.EXT_USB3_pwr,1);
	//light on usb pr led;
	gpio_direction_output(update_fw.USB_led,1);
	//set up timmer 
	init_timer(&update_fw.update_fw_timer);
	update_fw.update_fw_timer.function = timer_function;
	update_fw.update_fw_timer.expires= jiffies + msecs_to_jiffies(100);
	add_timer(&update_fw.update_fw_timer);
	return 0;
}

static void __exit update_fw_exit(void)
{
	gpio_set_value(update_fw.INFO_led,0);
	gpio_set_value(update_fw.PWR_led,0);
	//shutdwon EXT_USB3_pwr for security
	gpio_set_value(update_fw.EXT_USB3_pwr,0);
	gpio_set_value(update_fw.USB_led,0);
	gpio_free(update_fw.USB_led);
	gpio_free(update_fw.EXT_USB3_pwr);
	gpio_free(update_fw.INFO_led);
	gpio_free(update_fw.PWR_led);
	
	del_timer(&(update_fw.update_fw_timer));
	printk("update_fw:ByeBye-update_fw\n");
}

module_init(update_fw_init);
module_exit(update_fw_exit);
