typedef enum {
        BOOT_CONSOLE_MODE = 0,
        BOOT_RESCUE_MODE,
        BOOT_NORMAL_MODE,
        BOOT_MANUAL_MODE,
        BOOT_QC_VERIFY_MODE,
        BOOT_FLASH_WRITER_MODE,
        //regist new method
        BOOT_TFTP_KERN_MODE,
        //regist new method
        BOOT_TFTP_UBOT_MODE,
        BOOT_MODE_NUM,  
        BOOT_ANDROID_MODE,      
		BOOT_KERNEL_ONLY_MODE	
} BOOT_MODE;
