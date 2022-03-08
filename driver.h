/*
__init signals to the compiler that this function is only run once on
initialisation.

__exit signals that is is only run once when the module is removed.

static means that the function is not visible ouside of this file.
*/

// This stuff should go in the .h file


static int __init hcsr04_init(void);
static void __exit hcsr04_exit(void);

static int hcsr04_open(struct inode *inode, struct file *file);
static int hcsr04_release(struct inode *inode, struct file *file);
static ssize_t hcsr04_read(
    struct file *filp, char __user *buffer,
    size_t length, loff_t *offset);
static ssize_t hcsr04_write(
    struct file *filp, const char *buffer,
    size_t length, loff_t *offset);

/*
See https://tldp.org/LDP/lkmpg/2.4/html/c577.htm#:~:text=The%20file_operations%20Structure,to%20handle%20a%20requested%20operation.

As we do not need to do anything when the file is opened, we can ommit it.
We also do not care what happends when the release the device.
*/

static struct file_operations fops =
{
    .owner      = THIS_MODULE,
    .read       = hcsr04_read,
    .write      = hcsr04_write,
    .open       = hcsr04_open,
    .release    = hcsr04_release,
};