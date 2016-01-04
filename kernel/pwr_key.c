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
#define GPIOSYS "/sys/class/gpio/shutdown_low"

MODULE_AUTHOR("Danza");
MODULE_LICENSE("Dual BSD/GPL");

static struct workqueue_struct *pwr_key_workqueue; 
static struct work_struct shutdown_work;
static int do_shutdown(void *);

static struct file *file =NULL;

char path[ ] = "/sbin/halt";
char* argv[ ] = {path,NULL};
char* envp[ ] = {"HOME=/root","PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin",NULL};
char buf[8];	

struct pwr_key{
	unsigned int gpio2Irq;
	unsigned int pwrButton;
	unsigned int pwrLED;
	unsigned int pwrCPUOFF;
	unsigned int EXT_USB3_pwr;
	unsigned int USB_led;
	unsigned int SYS_led;
	unsigned int SYS_led_value;
	struct timer_list pwr_key_timmer;
	struct timer_list sys_timer;
	unsigned int ledValue;
}pwr_key;

static int do_shutdown(void *arg)
{
	int ret;
	mm_segment_t old_fs;
	printk(KERN_ALERT "pwr_key: System is going to shutdown!\n");
//	mdelay(1000);
	//open file .No Creating "shutdown_low" file
	file = filp_open(GPIOSYS,O_RDWR | O_APPEND ,0644);
	if(IS_ERR(file))
	{
		printk(KERN_ALERT "pwr.key: Get filp error!!\n");
		return 0;
	}
	//fill the value of CPUOFF pin num to shutdown_low in sysfs.
	buf[0] = 48 + pwr_key.pwrCPUOFF/10;
	buf[1] = 48 + pwr_key.pwrCPUOFF%10;
	old_fs = get_fs();
	set_fs(KERNEL_DS);
	file->f_op->write(file,(char*)buf,sizeof(buf),&file->f_pos);
	filp_close(file,NULL);
	set_fs(old_fs);
	mdelay(500);
	//gpio_set_value(pwr_key.pwrLED,!(pwr_key.ledValue));
	gpio_set_value(pwr_key.USB_led,0);
	gpio_direction_output(pwr_key.EXT_USB3_pwr,0);
	ret = call_usermodehelper(path,argv,envp,UMH_WAIT_PROC);
	//printk(KERN_ALERT "ret_value:%d\n",ret);
	return 0;
}	


static irqreturn_t poweroff(int irq, void *dev_id)
{
	int flag;
	flag = gpio_get_value(pwr_key.pwrButton);
	if (flag == 1)
	{
		mod_timer(&pwr_key.pwr_key_timmer,jiffies+msecs_to_jiffies(1000));
	}
	return IRQ_HANDLED;
}
static void sys_timer_function(unsigned long data)
{
		pwr_key.SYS_led_value = !(pwr_key.SYS_led_value);

		gpio_set_value(pwr_key.SYS_led,pwr_key.SYS_led_value);
		mod_timer(&pwr_key.sys_timer,jiffies+msecs_to_jiffies(1000));

}

static void timer_function(unsigned long data)
{
	int flag;
	flag = gpio_get_value(pwr_key.pwrButton);
	if(flag == 1)
	{
		printk(KERN_ALERT "\npwr_key :Timer prepare to shutdown!\n"); 
		printk("pwr_key: call workq to do_work of shutdown process!\n");
		schedule_work(&shutdown_work);
	}
	else
		printk(KERN_ALERT"pwr_key: Pwr_key is inited success!!\n");
}

static int __init pwr_key_init(void)
{
	struct device_node *p_node = NULL;
	int err;
//	int retv;
	pwr_key.ledValue = 1;
	pwr_key.SYS_led_value = 1;
	printk("init pwr_key module\n");
	pwr_key_workqueue = create_singlethread_workqueue("workq");
	INIT_WORK(&shutdown_work,do_shutdown);
	p_node = of_find_node_by_path("/pwr_key");   //遍历DTB 以path：/pwr_key的名字去寻找
	if ( p_node == NULL) {
		printk("pwr_key is in rescue mode cause of find no FDT node!\n");
		return -1;
		//根据p_node的返回值去判断是rescue mode 还是normal启动；
	}
	printk("pwr.key find FDT node success!\n");
	if (p_node != NULL) {
		pwr_key.pwrButton =  of_get_named_gpio(p_node,"pwrButton",0);
		pwr_key.pwrLED = of_get_named_gpio(p_node,"pwrLED",0);
		pwr_key.pwrCPUOFF = of_get_named_gpio(p_node,"pwrCPUOFF",0);
		pwr_key.EXT_USB3_pwr = of_get_named_gpio(p_node,"EXT_USB3_pwr",0);
		pwr_key.USB_led = of_get_named_gpio(p_node,"USB_led",0);
		pwr_key.SYS_led = of_get_named_gpio(p_node,"SYS_led",0);
	}
	printk("pwr_key.pwrButton is: gpio%d\n",pwr_key.pwrButton);
	printk("pwr_key.pwrLED is gpio:%d\n",pwr_key.pwrLED);
	printk("pwr_key.pwrCPUOFF is gpio:%d\n",pwr_key.pwrCPUOFF);
	printk("Pwr_key.EXT_USB3_pwr is gpio:%d\n",pwr_key.EXT_USB3_pwr);
	printk("Pwr_key.USB_led is gpio:%d\n",pwr_key.USB_led);
	printk("Pwr_key.SYS_led is gpio:%d\n",pwr_key.SYS_led);
//	retv = gpio_request(pwr_key.pwrButton,NULL);
//	printk(KERN_ALERT"\n retv=%d\n",retv);
	if(gpio_request(pwr_key.pwrButton,NULL) ||gpio_request(pwr_key.pwrLED,NULL) || gpio_request(pwr_key.EXT_USB3_pwr,NULL) || gpio_request(pwr_key.USB_led,NULL) || gpio_request(pwr_key.SYS_led,NULL))
	{
		gpio_free(pwr_key.pwrButton);
		gpio_free(pwr_key.pwrLED);
		gpio_free(pwr_key.EXT_USB3_pwr);
		gpio_free(pwr_key.USB_led);
		gpio_free(pwr_key.SYS_led);
		printk("pwr_key:bad gpio_request!!\n");
		return -2;
	}

	gpio_direction_input(pwr_key.pwrButton);
	gpio_direction_output(pwr_key.pwrLED,pwr_key.ledValue);
	mdelay(1000);

	// turn usb2.0 power for mele board.
	gpio_direction_output(pwr_key.EXT_USB3_pwr,1);
	//turn usb led pwr led for mele board.
	gpio_direction_output(pwr_key.USB_led,1);
	mdelay(1000);

	printk(KERN_ALERT"pwr_key: Switch on WIFI&USB2 port power\n");

	init_timer(&pwr_key.pwr_key_timmer);
	pwr_key.pwr_key_timmer.function = timer_function;
	pwr_key.pwr_key_timmer.expires= jiffies + msecs_to_jiffies(200);
	add_timer(&pwr_key.pwr_key_timmer);

	gpio_direction_output(pwr_key.SYS_led,pwr_key.SYS_led_value);
	init_timer(&pwr_key.sys_timer);
	pwr_key.sys_timer.function = sys_timer_function;
	pwr_key.sys_timer.expires = jiffies + msecs_to_jiffies(1000);
	add_timer(&pwr_key.sys_timer);

	pwr_key.gpio2Irq = gpio_to_irq(pwr_key.pwrButton);
	err = request_irq(pwr_key.gpio2Irq,poweroff,IRQF_TRIGGER_RISING,"pwr_key",NULL);
	printk("pwr_key: request_irq results : %d\n",err);
	return 0;
}	

static void __exit pwr_key_exit(void)
{
	printk("pwr_key: BYEBYE!\n");
	free_irq(pwr_key.gpio2Irq, NULL);
	gpio_free(pwr_key.pwrButton);
	gpio_free(pwr_key.pwrLED);
	gpio_free(pwr_key.SYS_led);
	destroy_workqueue(pwr_key_workqueue);
	del_timer(&pwr_key.pwr_key_timmer);
	del_timer(&pwr_key.sys_timer);
}	

module_init(pwr_key_init);
module_exit(pwr_key_exit);
