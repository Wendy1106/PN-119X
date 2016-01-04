*************************************************************************************
//Define the ASCII encoding
#define _TAB			0x09
#define _ENTER			0x0D
#define _ESC			0x1B
#define _SPACE			0x20
#define _KKEY			0x6B
#define _BKEY			0x62

*************************************************************************************
			case _KKEY:
				printf("\nPress k key for auto kernel download!\n");
				boot_mode  = BOOT_TFTP_KERN_MODE;
				abort = 1;
				break;
			case _BKEY:
				printf("\nPress b key for auto uboot update mode!\n");
				boot_mode = BOOT_TFTP_UBOT_MODE;
				abort = 1;
				break;	
			case _ENTER:
			default:
				debug("Press Key is %c\n", console_key); 
				/* do nothing */
				break;
*************************************************************************************
#ifdef  AUTO_TFTP
	else if(boot_mode == BOOT_TFTP_UBOT_MODE )
	{
		WATCHDOG_DISABLE();
		printf("\nThis is tftp mode for uboot auto flash by danza\n");
		printf("Starting tftp process...........\n");
		run_command_list("env set ipaddr 192.168.0.9",-1,0);
		run_command_list("env set serverip 192.168.0.188",-1,0);
		run_command_list("env set gatewayip 192.168.0.1",-1,0);
		run_command_list("dcache off",-1,0);
		run_command_list("tftp 0x01500000 dvrboot.exe.bin",-1,0);
		printf("uboot binary down load success!\n");
		run_command_list("dcache on",-1,0);
		run_command_list("go 0x01500000",-1,0);

	}
	else if(boot_mode == BOOT_TFTP_KERN_MODE )
	{
		WATCHDOG_DISABLE();
		printf("\nThis is tftp mode by danza\n");
		printf("Starting tftp process...........\n");
		run_command_list("env set ipaddr 192.168.0.9",-1,0);
		run_command_list("env set serverip 192.168.0.188",-1,0);
		run_command_list("env set gatewayip 192.168.0.1",-1,0);
		run_command_list("dcache off",-1,0);
		run_command_list("tftp  0x01ff2000 rescue.emmc.dtb",-1,0);
		run_command_list("tftp  0x3000000 emmc.uImage",-1,0);
		run_command_list("tftp  0x02200000 rescue.root.emmc.cpio.gz_pad.img",-1,0);
		printf("kernel fdt download success!\n");
		run_command_list("dcache on",-1,0);
		run_command_list("go k",-1,0);
	}
#endif

#endif /* CONFIG_BSP_REALTEK */
*************************************************************************************
