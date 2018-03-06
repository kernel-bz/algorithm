//
//	file name	: km_misc.c
//	author		: Jung,JaeJoon(rgbi3307@nate.com) on the www.kernel.bz
//	comments	: kernel misc driver
//				  kernel version 2.6.31
//

#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/miscdevice.h>

#include <linux/kfifo.h>
#include <linux/spinlock.h>
#include <linux/spinlock_types.h>
#include <linux/err.h>
#include <linux/slab.h>		//kmalloc

int fn01_kfifo_test(void)
{
	struct kfifo *kfifo;
	unsigned char buf[] = {"This is kfifo test function named fn01_kfifo_test() in the my_misc.\n\0"};
	unsigned char *buf2, c;
	int ret, err=0;
	unsigned int i, bsize;
	spinlock_t my_lock;

	spin_lock_init(&my_lock);
	//PAGE_SIZE==4KB
	kfifo = kfifo_alloc(PAGE_SIZE, GFP_KERNEL, &my_lock);
	if (IS_ERR(kfifo)) return -1;	

	bsize = sizeof(buf)/sizeof(unsigned char);
	for (i = 0; i < bsize; i++)
		kfifo_put(kfifo, buf+i, sizeof(unsigned char));

	buf2 = kmalloc(bsize, GFP_KERNEL);
	i = 0;
	while (kfifo_len(kfifo)) {
		ret = kfifo_get(kfifo, &c, sizeof(c));
		if (ret != sizeof(c)) {
			err++;
			break;
		}
		*(buf2+i) = c;
		i++;
	}
	printk(KERN_INFO "my_misc: kfifo=%s", buf2);
	kfifo_free(kfifo);
	kfree(buf2);
	return err;
}

//------------------ kernel misc driver module --------------------------------
static int my_misc_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int my_misc_close(struct inode *inode, struct file *file)
{
	return 0;
}

static ssize_t my_misc_write(struct file *file, const char *data, size_t len, loff_t *ppose)
{
	return 0;
}

static int my_misc_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	switch (cmd) {
		case 0: fn01_kfifo_test(); 
			break;
		case 1: printk("my_misc_ioctrl(1)\n");
			break;
		case 2: printk("my_misc_ioctrl(2)\n");
			break;
		default: 
			return -1;
	}
	return 0;
}

struct file_operations my_misc_fops = {
	.owner = THIS_MODULE,
	.open = my_misc_open,
	.release = my_misc_close,
	.write = my_misc_write,
	.ioctl = my_misc_ioctl
};

static struct miscdevice my_misc_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "my_misc",
	.fops = &my_misc_fops
};

static int __init my_misc_init(void)
{
	misc_register(&my_misc_dev);
	printk("**my_misc driver init**\n");
	return 0;
}

static void __exit my_misc_exit(void)
{
	misc_deregister(&my_misc_dev);
	printk("**my_misc driver exit**\n");
}

module_init(my_misc_init);
module_exit(my_misc_exit);

MODULE_LICENSE("GPL");

