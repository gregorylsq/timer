#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_address.h>
#include <linux/timer.h>
// #include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#define TIMER_CNT   1
#define TIMER_NAME "timer"

#define CLOSE_CMD (_IO(0XEF,0x1))
#define OPEN_CMD  (_IO(0XEF,0x2))
#define SETPERIOD (_IO(0XEF,0x3))

struct timer_dev{
    dev_t devid;     //major deviceid and minor deviceid
    struct cdev cdev;
    struct class *class;
    struct device *device;
    int major;
    int minor;
    struct device_node *nd;
    struct semaphore sem;
    int timeperiod;
    struct timer_list timer;
    spinlock_t lock;
};

struct timer_dev timerdev;


static int timer_open(struct inode *inode,struct file *filp)
{
    filp->private_data=&timerdev;

    timerdev.timeperiod=1000;
}

static long timer_unlocked_ioctl(struct file *filp,unsigned int cmd, unsigned long arg)
{
    struct timer_dev *dev = (struct timer_dev *)filp->private_data;
    int timerperiod;
    unsigned long flags;


    switch (cmd)
    {
        case CLOSE_CMD:
            del_timer_sync(&dev->timer);
        break;

        case OPEN_CMD:
            spin_lock_irqsave(&dev->lock,flags);
            timerperiod=dev->timeperiod;
            spin_unlock_irqrestore(&dev->lock,flags);
            mod_timer(&dev->timer,jiffies+msecs_to_jiffies(timerperiod));
        break;

        case SETPERIOD:
            spin_lock_irqsave(&dev->lock,flags);
            dev->timeperiod=arg;
            spin_unlock_irqrestore(&dev->lock,flags);
            mod_timer(&dev->timer,jiffies+msecs_to_jiffies(arg));
    }
}