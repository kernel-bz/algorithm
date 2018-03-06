//
//	file name	: km_misc_run.c
//	author		: Jung,JaeJoon(rgbi3307@nate.com) on the www.kernel.bz
//	comments	: kernel misc driver run
//				  kernel version 2.6.31
//	

#include <fcntl.h>
#include <asm/types.h>
#include <unistd.h>

int main(void)
{
	int fd = open("/dev/my_misc", O_RDWR);

	ioctl(fd, 0, 0);
	fsync(fd);
	
	while(1) pause();

	return 0;
}
