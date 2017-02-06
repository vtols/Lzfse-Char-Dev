#include <linux/init.h>
#include <linux/module.h>

#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/lz4.h>

#include <asm/uaccess.h>

MODULE_LICENSE("Dual BSD/GPL");

#define LZFSE_MAJOR             566
#define LZFSE_MINOR_COMPRESS    0
#define LZFSE_MINOR_DECOMPRESS  1
#define BUFFER_SIZE             (4096*2)
#define SCRATCH_BUFFER          (1024*1024)

struct cdev compress, decompress;

char store_buf[BUFFER_SIZE], tmp_buf[BUFFER_SIZE], wrk[SCRATCH_BUFFER];
int w_mode = 0, wrote;
size_t buf_len = 0;

ssize_t lzfse_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
    printk(KERN_ALERT "READ\n");
    
    if (count > buf_len - *f_pos)
        count = buf_len - *f_pos;
    
    if (count > 0)
        copy_to_user(buf, &store_buf[*f_pos], count);
    
    *f_pos += count;
    
    return count;
}

ssize_t lzfse_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    printk(KERN_ALERT "WRITE\n");
    
    if (count > BUFFER_SIZE - *f_pos)
        count = BUFFER_SIZE - *f_pos;

    if (count > 0)
        copy_from_user(&tmp_buf[*f_pos], buf, count);

    buf_len = *f_pos + count;
    
    *f_pos += count;
    wrote = 1;
    
    return count;
}

int lzfse_compress_open(struct inode *inode, struct file *filp)
{
    printk(KERN_ALERT "COMPRESS OPEN\n");

    w_mode = 0;
    wrote = 0;

    return 0;
}

int lzfse_decompress_open(struct inode *inode, struct file *filp)
{
    printk(KERN_ALERT "DECOMPRESS OPEN\n");

    w_mode = 1;
    wrote = 0;
    
    return 0;
}

int lzfse_release(struct inode *inode, struct file *filp)
{
    int ret = 0;
    size_t new_buf_len = BUFFER_SIZE;
    
    printk(KERN_ALERT "RELEASE\n");
    printk(KERN_ALERT "WROTE: %d\n", wrote);


    if (wrote) {
        if (w_mode == 0) {
            printk(KERN_ALERT "COMPRESS\n");
            buf_len = lz4hc_compress(tmp_buf, buf_len, store_buf, &new_buf_len, wrk);
        } else {
            printk(KERN_ALERT "DECOMPRESS\n");
            ret = lz4_decompress_unknownoutputsize(tmp_buf, buf_len, store_buf, &new_buf_len);
        }
        if (ret == 0)
            buf_len = new_buf_len;
        else
            buf_len = 0;
    }


    printk(KERN_ALERT "RELEASE END\n");
    if (ret != 0) {
        printk(KERN_ALERT "COMPRESS/DECOMPRESS ERROR\n");
    }

	return 0;
}

struct file_operations lzfse_compress_fops = {
	.owner =    THIS_MODULE,
	.read =     lzfse_read,
	.write =    lzfse_write,
	.open =     lzfse_compress_open,
	.release =  lzfse_release,
};

struct file_operations lzfse_decompress_fops = {
	.owner =    THIS_MODULE,
	.read =     lzfse_read,
	.write =    lzfse_write,
	.open =     lzfse_decompress_open,
	.release =  lzfse_release,
};

static int lzfse_init(void)
{
    dev_t dev;
    
    printk(KERN_ALERT "Hello, world\n");
    
    dev = MKDEV(LZFSE_MAJOR, 0);
    register_chrdev_region(dev, 2, "lzfse");
    
    
    dev = MKDEV(LZFSE_MAJOR, LZFSE_MINOR_COMPRESS);
    cdev_init(&compress, &lzfse_compress_fops);
	compress.owner = THIS_MODULE;
	compress.ops = &lzfse_compress_fops;
	cdev_add(&compress, dev, 1);

    dev = MKDEV(LZFSE_MAJOR, LZFSE_MINOR_DECOMPRESS);
    cdev_init(&decompress, &lzfse_decompress_fops);
	decompress.owner = THIS_MODULE;
	decompress.ops = &lzfse_decompress_fops;
	cdev_add(&decompress, dev, 1);
    
    return 0;
}

static void lzfse_exit(void)
{
    printk(KERN_ALERT "Goodbye, cruel world\n");
}

module_init(lzfse_init);
module_exit(lzfse_exit);
