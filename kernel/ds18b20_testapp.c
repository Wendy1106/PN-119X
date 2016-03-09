#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/ioctl.h>

int main(void)
{
	int fd;
	unsigned char result[2];    // 从ds18b20读出的结果，result[0]存放低八位
	unsigned char integer_value = 0;
	float decimal_value = 0;    // 温度数值,decimal_value为小数部分的值
	float temperature = 0;
  
	fd = open("/dev/ds18b20", 0);
	if (fd < 0) {
		perror("open device failed\n");
		exit(1);
	}

	while (1) {
		read(fd, &result, sizeof(result));
		integer_value = ((result[0] & 0xf0) >> 4) | ((result[1] & 0x07) << 4);
		// 精确到0.25度
		decimal_value = 0.5 * ((result[0] & 0x0f) >> 3) + 0.25 * ((result[0] & 0x07) >> 2);
		temperature = (float)integer_value + decimal_value;
		printf("%6.2f\n", temperature);
		sleep(4);
	}
	return 0;
}