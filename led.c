#include <linux/init.h>           // Macros used to mark up functions e.g. __init __exit
#include <linux/module.h>         // Core header for loading LKMs into the kernel
#include <linux/device.h>         // Header to support the kernel Driver Model
#include <linux/kernel.h>         // Contains types, macros, functions for the kernel
#include <linux/fs.h>             // Header for the Linux file system support
#include <linux/uaccess.h>          // Required for the copy to user function
#include <linux/tty.h>
#include <linux/kd.h>
#include <linux/console_struct.h>
#include <linux/slab.h>

extern int fg_console;

static char *DEVICE_NAME = "led";
static char *CLASS_NAME = "LEDClass";                           
static short state;
static int major;
static struct class* LEDCharClass;
static struct device* LEDCharDevice;

/*
 *Function to turn LED light on keyboard on and off 
 *writing on to file turns it on
 *writing off turns it off
 *reseting it resets the light
*/
static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset){
	struct tty_driver *my_driver;
	char *s_ptr = kmalloc(sizeof(char) * (len - 1), GFP_KERNEL);
	int error_code = 0;
	error_code = copy_from_user(s_ptr,buffer, len - 1);
	printk(KERN_ALERT "The file was written\n");
	printk(KERN_ALERT "%s\n", s_ptr);
	my_driver = vc_cons[fg_console].d->port.tty->driver;
	if(strncmp(s_ptr,"on", len - 1) == 0){
		printk(KERN_ALERT "Turning LED On\n");
		state |= 1 << 2;
		((my_driver->ops)->ioctl) (vc_cons[fg_console].d->port.tty, KDSETLED, state);
	}else if(strncmp(s_ptr,"off", len - 1) == 0){
		printk(KERN_ALERT "Turning LED Off\n");
		state = state & ~(1 << 2);
		printk(KERN_ALERT "%d\n",state);
		((my_driver->ops)->ioctl) (vc_cons[fg_console].d->port.tty, KDSETLED, state);
	}else if(strncmp(s_ptr,"reset", len - 1) == 0){
		state = 0;
		printk(KERN_ALERT "Reseting LED\n");
		((my_driver->ops)->ioctl) (vc_cons[fg_console].d->port.tty, KDSETLED, 0xFF);
	}
	kfree(s_ptr);
    return len;
}
/*
 * Function to get current value of light 
 * prints an on lightbulb if light is on
 * prints an off lightbulb if light is off
 * 
 */
static ssize_t device_read(struct file *flip, char *buff, size_t length, loff_t *offset){
	const char *s_ptr;
	int error_code = 0;
	ssize_t len = 0;
	
	if(((state & ( 1 << 2 )) >> 2) == 0){
		s_ptr = " _\n(.)\n =\n";
	}else{
		s_ptr = " _\n(*)\n =\n";
	}
	
	len = min( (unsigned long)(strlen(s_ptr) - *offset), (unsigned long)(length));
	if(len <= 0){
		return 0;
	}
	
	error_code = copy_to_user(buff, s_ptr + *offset, len);
	printk(KERN_ALERT "The file was read");
	printk(KERN_ALERT "Error code %d", error_code);
	
	*offset += len;
	return len;
}
//File ops
static struct file_operations fops = {
	.read = device_read,
	.write = dev_write
};

//Regester char dev and set variables
static int __init start(void){
	state = 0;
	major = register_chrdev(0,DEVICE_NAME, &fops);
	if(major < 0){
		printk(KERN_ALERT "FAILED TO REG LED\n");
		return major;
	}
	printk(KERN_ALERT "Led created with major %d\n",major);
	LEDCharClass = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(LEDCharClass)){                // Check for error and clean up if there is
      unregister_chrdev(major, DEVICE_NAME);
      printk(KERN_ALERT "Failed to register device class\n");
      return PTR_ERR(LEDCharClass);
    }
    LEDCharDevice = device_create(LEDCharClass, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);
    if (IS_ERR(LEDCharDevice)){               // Clean up if there is an error
      class_destroy(LEDCharClass);           // Repeated code but the alternative is goto statements
      unregister_chrdev(major, DEVICE_NAME);
      printk(KERN_ALERT "Failed to create the device\n");
      return PTR_ERR(LEDCharDevice);
    }
	return 0;
}


//Clean up and unregester
static void __exit end(void){
	printk("Led module unloaded");
	device_destroy(LEDCharClass, MKDEV(major, 0));
	class_unregister(LEDCharClass);
	class_destroy(LEDCharClass);
	unregister_chrdev(major, DEVICE_NAME);
}

module_init(start);
module_exit(end);

MODULE_LICENSE("GPL");
