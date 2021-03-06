#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>
#include <linux/timer.h>

#include "hcsr04_sensor.h"

#define DRIVER_AUTHOR "Edward Jex <edward.j.jex@durham.ac.uk>"
#define DRIVER_DECSRIPTION "A driver to read the HC-SR04 sensor.\nWriting to the device file using the format <ECHO/TRIG> <GPIO PIN (2 digits)> can be used to change the pin settings.\nReading from the device file returns the bytes of an unsigned long containg the time in microseconds for a single roundtrip."
#define DRIVER_VERSION "1.0.0"

#define MAX_BUFFER_LENGTH 7
#define TRIGGER_HIGH_TIME_uS 10
#define SPEED_OF_SOUND_M_S 343

// Never going to be able to write more than 100 as we are taking 2
// chars of input.
static uint8_t echo_pin = 100;
static uint8_t trig_pin = 100;

/*
The open and release functions are not currently needed but are there to
make the code more readable. May be used in future.
*/

int hcsr04_open(struct inode *inode, struct file *file)
{
    pr_info("Opened device file\n");
    return 0;
}

int hcsr04_release(struct inode *inode, struct file *file)
{
    pr_info("Closed device file\n");
    return 0;
}

/*
The read function is used to read the distance from the sensor.
*/

ssize_t hcsr04_read(
    struct file *filp, char __user *buffer,
    size_t length, loff_t *offset)
{
    long timeout_us;
    unsigned long elapsed_us;

    ktime_t timeout, start, finish;

    if (*offset) {
        return 0;
    }

    pr_info("Attempting to read sensor\n");

    if (echo_pin >= 100 || trig_pin >= 100)
    {
        pr_err("GPIO pins have not been defined yet\n");
        return 0;
    }

    // ktime_t timeout = ktime_us

    // Send trugger pulse
    gpio_set_value(trig_pin, 1);
    udelay(TRIGGER_HIGH_TIME_uS);
    gpio_set_value(trig_pin, 0);

    // Pulse round trip is a maximum of 8 meters
    // 8 pulses are required
    // Add a 200% margin for error just in case and because it doesn't
    // work without it.

    // This waiting is blocking but seems like the most accurate way

    timeout_us = 1e9;

    timeout = ktime_add_us(ktime_get(), timeout_us);

    // Look at timers maybe
    pr_info("Waiting for pin to go high");
    while (!gpio_get_value(echo_pin))
    {
        if (ktime_compare(ktime_get(), timeout) >= 0)
        {
            elapsed_us = ULONG_MAX;
            pr_err("Timeout reading the sensor, no object detected\n");
            goto timeout;
        }
    }
    start = ktime_get();
    while (gpio_get_value(echo_pin))
    {
        if (ktime_compare(ktime_get(), timeout) >= 0)
        {
            elapsed_us = ULONG_MAX;
            pr_err("Timeout reading the sensor, no object detected\n");
            goto timeout;
        }
    }


    finish = ktime_get();
    elapsed_us = (unsigned long)ktime_us_delta(finish, start);

    pr_info("Read the sensor, got a total round trip time of %lu us\n", elapsed_us);
timeout:
    length = sizeof(unsigned long);

    if (copy_to_user(buffer, &elapsed_us, length))
    {
        pr_err("Error copying elapsed time to user\n");
    }

    *offset += length;
    return length;
}

/*
The write function is used to set the GPIO pins for the sensor.

Expect input to be like <ECHO/TRIG> <GPIO pin>
e.g.
EHCO 3
TRIG 21
*/

ssize_t hcsr04_write(
    struct file *filp, const char *buffer,
    size_t length, loff_t *offset)
{
    uint8_t internal_buffer[MAX_BUFFER_LENGTH];
    char pin_type[5];
    char pin_no_str[3];
    long pin_no;
    size_t written;

    pr_info("Attempting to change GPIO pins\n");
    // if (length > MAX_BUFFER_LENGTH)
    // {
    //     pr_err(
    //         "Data passed to driver was too large for the buffer. Expecting input of the form '<ECHO/TRIG> <GPIO pin>'\n");
    // }

    if (length < MAX_BUFFER_LENGTH) {
        written = length;
    } else {
        written = MAX_BUFFER_LENGTH;
    }

    memset(internal_buffer, 0, MAX_BUFFER_LENGTH);
    memset(pin_type, 0, 5);
    memset(pin_no_str, 0, 3);
    // copy_from_user returns the number of bytes that were unable to
    // be copied
    
    if (copy_from_user(internal_buffer, buffer, MAX_BUFFER_LENGTH))
    {
        pr_err("No all bytes were copied from user\n");
    }

    memcpy(pin_type, internal_buffer, 4);
    memcpy(pin_no_str, internal_buffer + 5, 2);

    if (kstrtol(pin_no_str, 10, &pin_no))
    {
        pr_err("Error whist parsing input, GPIO pin should be a number, got %s\n", pin_no_str);
        return written;
    }

    if (!gpio_is_valid(pin_no))
    {
        pr_err("Error whist parsing input, GPIO pin is invalid'\n");
        return written;
    }

    if (!strcasecmp(pin_type, "ECHO"))
    {
        if (echo_pin == pin_no) {
            pr_info("Echo pin already set to %d\n", echo_pin);
            return written;
        }
        if (gpio_request(pin_no, "ECHO_PIN") || gpio_direction_input(pin_no))
        {
            pr_err("Error when requesting GPIO pin");
            return written;
        }
        echo_pin = pin_no;
        pr_info("Echo has been set to GPIO pin %d\n", echo_pin);
    }
    else if (!strcasecmp(pin_type, "TRIG"))
    {
        if (trig_pin == pin_no) {
            pr_info("Trig pin already set to %d\n", trig_pin);
            return written;
        }
        if (gpio_request(pin_no, "TRIG_PIN") || gpio_direction_output(pin_no, 0))
        {
            pr_err("Error when requesting GPIO pin");
            return written;
        }
        trig_pin = pin_no;
        pr_info("Trigger has been set to GPIO pin %d\n", trig_pin);
    }
    else
    {
        pr_err("Error whist parsing input, Expecting input of the form '<ECHO/TRIG> <GPIO pin>'\n");
        pr_err("Got %s %ld\n", pin_type, pin_no);
        return written;
    }

    return written;
}

// Note: Look at add_timer, init_timer, del_tiner
int __init hcsr04_init(void)
{
    // Get major number
    if ((alloc_chrdev_region(&dev, 0, 1, "hcsr04_dev")) < 0)
    {
        pr_err("Failed to allocate device numbers\n");
        goto failed_allocate_dev_numbers;
    }

    // Create character device structure
    cdev_init(&hcsr04_cdev, &fops);

    // Add the character device to the kernel
    if ((cdev_add(&hcsr04_cdev, dev, 1)) < 0)
    {
        pr_err("Falied to add the device\n");
        goto failed_add_device;
    }

    dev_class = class_create(THIS_MODULE, "hcsr04_class");

    if (!dev_class)
    {
        pr_err("Failed to create the device class\n");
        goto failed_create_class;
    }

    if (!device_create(dev_class, NULL, dev, NULL, "hcsr04"))
    {
        pr_err("Failed to create the device\n");
        goto failed_create_device;
    }

    return 0;

failed_create_device:
    device_destroy(dev_class, dev);

failed_create_class:
    class_destroy(dev_class);

failed_add_device:
    cdev_del(&hcsr04_cdev);

failed_allocate_dev_numbers:
    unregister_chrdev_region(dev, 1);

    return 1;
}

void __exit hcsr04_exit(void)
{
    if (trig_pin < 100)
    {
        gpio_free(trig_pin);
    }

    if (echo_pin < 100)
    {
        gpio_free(echo_pin);
    }

    device_destroy(dev_class, dev);
    class_destroy(dev_class);
    cdev_del(&hcsr04_cdev);
    unregister_chrdev_region(dev, 1);

    return;
}

module_init(hcsr04_init);
module_exit(hcsr04_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DECSRIPTION);
MODULE_VERSION(DRIVER_VERSION);