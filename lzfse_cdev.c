#include <linux/init.h>
#include <linux/module.h>

#include <linux/string.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/crypto.h>

#include <asm/uaccess.h>

MODULE_LICENSE("Dual BSD/GPL");

#define LZFSE_MAJOR             566
#define LZFSE_MINOR_COMPRESS    0
#define LZFSE_MINOR_DECOMPRESS  1
#define LZFSE_MINOR_OPTION      2
#define BUFFER_SIZE             (60*1024*1024)
#define MARK_SIZE               sizeof(size_t)

struct cdev compress, decompress, options;

char store_buf[BUFFER_SIZE], tmp_buf[BUFFER_SIZE];
int w_mode = 0, wrote;
size_t buf_len = 0;
size_t block_size = 4096;
char algo[10];

ssize_t lzfse_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
    printk(KERN_ALERT "READ\n");
    
    if (count > buf_len - *f_pos)
        count = buf_len - *f_pos;
    
    if (count > 0)
        copy_to_user(buf, store_buf + *f_pos, count);
    
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
        copy_from_user(tmp_buf + *f_pos, buf, count);

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

int lzfse_opt_open(struct inode *inode, struct file *filp)
{
    printk(KERN_ALERT "OPTION OPEN\n");
    
    return 0;
}

int lzfse_release(struct inode *inode, struct file *filp)
{
    struct crypto_comp *tfm;

    int ret = 0;
    unsigned int new_buf_len = 0, to_decode, out_len;
    char *r_pos = tmp_buf, *w_pos = store_buf;
    
    printk(KERN_ALERT "RELEASE\n");
    printk(KERN_ALERT "WROTE: %d\n", wrote);


    if (wrote) {
        tfm = crypto_alloc_comp("lz4hc", 0, 0);

        if (!IS_ERR(tfm)) {
            if (w_mode == 0) {
                printk(KERN_ALERT "COMPRESS, %s by %zu bytes blocks\n",
                    algo, block_size);
                
                while (buf_len > 0) {
                    to_decode = min(block_size, buf_len);
                    buf_len -= to_decode;
                    out_len = BUFFER_SIZE;
                    ret = crypto_comp_compress(tfm, r_pos, to_decode,
                            w_pos + MARK_SIZE, &out_len);
                    if (ret != 0) {
                        new_buf_len = 0;
                        break;
                    }
                    r_pos += to_decode;
                    *((size_t *) w_pos) = out_len;
                    w_pos += MARK_SIZE + out_len;
                    new_buf_len += MARK_SIZE + out_len;
                }
                *((size_t *) w_pos) = 0;
            } else {
                printk(KERN_ALERT "DECOMPRESS %s\n", algo);
                
                while (buf_len > 0) {
                    to_decode = *((size_t *) r_pos);
                    buf_len -= MARK_SIZE + to_decode;
                    out_len = BUFFER_SIZE;
                    ret = crypto_comp_decompress(tfm, r_pos + MARK_SIZE,
                            to_decode, w_pos, &out_len);
                    if (ret != 0) {
                        new_buf_len = 0;
                        break;
                    }
                    r_pos += MARK_SIZE + to_decode;
                    w_pos += out_len;
                    new_buf_len += out_len;
                }
            }
            buf_len = new_buf_len;

            crypto_free_comp(tfm);
        } else {
            ret = -1;
            printk(KERN_ALERT "CAN'T ALLOC COMPRESS ALGO\n");
        }
    }


    printk(KERN_ALERT "RELEASE END\n");
    if (ret != 0) {
        printk(KERN_ALERT "COMPRESS/DECOMPRESS ERROR\n");
    }

	return ret;
}

int lzfse_opt_release(struct inode *inode, struct file *filp)
{
    sscanf(tmp_buf, "%zu %s", &block_size, &algo);
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

struct file_operations lzfse_opt_fops = {
	.owner =    THIS_MODULE,
	.open =     lzfse_opt_open,
	.write =    lzfse_write,
	.release =  lzfse_opt_release,
};

static int lzfse_init(void)
{
    dev_t dev;
    
    printk(KERN_ALERT "Hello, world\n");
    
    dev = MKDEV(LZFSE_MAJOR, 0);
    register_chrdev_region(dev, 3, "lzfse");
    
    
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
    
    dev = MKDEV(LZFSE_MAJOR, LZFSE_MINOR_OPTION);
    cdev_init(&options, &lzfse_opt_fops);
	options.owner = THIS_MODULE;
	options.ops = &lzfse_opt_fops;
	cdev_add(&options, dev, 1);
    
    strcpy(algo, "lzo");
    
    return 0;
}

static void lzfse_exit(void)
{
    printk(KERN_ALERT "Goodbye, cruel world\n");
}

module_init(lzfse_init);
module_exit(lzfse_exit);
