#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/string.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "mychardev"
#define BUFFER_SIZE 1024

static int major;
static char device_buffer[BUFFER_SIZE] = "Hello from Kernel Driver\n";
static size_t data_size = sizeof("Hello from Kernel Driver\n") - 1;
static DEFINE_MUTEX(device_lock);

static int dev_open(struct inode *inode, struct file *file)
{
    pr_info("%s: device opened\n", DEVICE_NAME);
    return 0;
}

static int dev_release(struct inode *inode, struct file *file)
{
    pr_info("%s: device closed\n", DEVICE_NAME);
    return 0;
}

static ssize_t dev_read(struct file *file, char __user *user_buffer,
                        size_t len, loff_t *offset)
{
    size_t bytes_to_read;

    if (mutex_lock_interruptible(&device_lock))
        return -ERESTARTSYS;

    if (*offset >= data_size) {
        mutex_unlock(&device_lock);
        return 0;
    }

    bytes_to_read = min(len, data_size - (size_t)*offset);

    if (copy_to_user(user_buffer, device_buffer + *offset, bytes_to_read)) {
        mutex_unlock(&device_lock);
        return -EFAULT;
    }

    *offset += bytes_to_read;
    mutex_unlock(&device_lock);

    return bytes_to_read;
}

static ssize_t dev_write(struct file *file, const char __user *user_buffer,
                         size_t len, loff_t *offset)
{
    size_t bytes_to_write = min(len, (size_t)(BUFFER_SIZE - 1));

    if (mutex_lock_interruptible(&device_lock))
        return -ERESTARTSYS;

    if (copy_from_user(device_buffer, user_buffer, bytes_to_write)) {
        mutex_unlock(&device_lock);
        return -EFAULT;
    }

    device_buffer[bytes_to_write] = '\0';
    data_size = bytes_to_write;
    mutex_unlock(&device_lock);

    pr_info("%s: %zu bytes written\n", DEVICE_NAME, bytes_to_write);
    return bytes_to_write;
}

static const struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = dev_open,
    .read = dev_read,
    .write = dev_write,
    .release = dev_release,
};

static int __init char_driver_init(void)
{
    major = register_chrdev(0, DEVICE_NAME, &fops);
    if (major < 0) {
        pr_err("%s: registration failed: %d\n", DEVICE_NAME, major);
        return major;
    }

    pr_info("%s: registered with major number %d\n", DEVICE_NAME, major);
    return 0;
}

static void __exit char_driver_exit(void)
{
    unregister_chrdev(major, DEVICE_NAME);
    pr_info("%s: unregistered\n", DEVICE_NAME);
}

module_init(char_driver_init);
module_exit(char_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("HameedRahman119");
MODULE_DESCRIPTION("Simple Linux character driver with read/write support");
MODULE_VERSION("1.1");
