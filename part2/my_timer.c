#include <linux/init.h>
#include <linux/module.h>
#include <linux/timekeeping.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h> //file sys calls
#include <linux/uaccess.h> //memory copy from kernel <...> userspace
#include <linux/ktime.h> //for ktime_get_real_ts64
#include <linux/seq_file.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Group 24");
MODULE_DESCRIPTION("Tell Time Since Last Epoch Kernel Module");
MODULE_VERSION("1.0");

#define ENTRY_NAME "timer"
#define PERMS 0666
#define PARENT NULL

#define BUF_LEN 100 //Max length of read/write message//
static struct proc_dir_entry* proc_entry; //pointer to proc entry

static struct timespec64 lt; //last-saved time

int len;

static ssize_t procfile_read(struct file* file, char* ubuf, size_t count, loff_t *ppos)
{
	struct timespec64 ct, et; //current time and elapsed time//
	ktime_get_real_ts64(&ct);
	char buffer[128];
	
	//No elapsed time has been recorded//
	if (lt.tv_sec == 0)
	{
		len = snprintf(buffer, sizeof(buffer), "current time: %lld.%ld\n",
		 ct.tv_sec, ct.tv_nsec);
        	if (*ppos > 0 || count < len)
        	{
                	return 0;
        	}

        	if (len >= 0)
        	{
                	if(copy_to_user(ubuf, buffer, len))
                	{
                        	printk(KERN_INFO "Error copying to user\n");
                	}
        	}
	}
	//Elapsed time has been recorded, print both//
	else if (lt.tv_sec != 0)
	{
		ktime_get_real_ts64(&ct);
		et = timespec64_sub(ct, lt);

		len = snprintf(buffer, sizeof(buffer), 
		"current time: %lld.%ld\nelapsed time: %lld.%ld\n", 
		ct.tv_sec, ct.tv_nsec, et.tv_sec, et.tv_nsec);

                if (*ppos > 0 || count < len)
                {
                        return 0;
                }

                if (len >= 0)
                {
                        if(copy_to_user(ubuf, buffer, len))
                        {
                                printk(KERN_INFO "Error copying to user\n");
                        }
                }
	}
	lt = ct;
	*ppos = len;
	return len;
};

static const struct proc_ops procfile_fops = {
	.proc_read = procfile_read
};

static int __init timer_init(void) {
	proc_entry = proc_create(ENTRY_NAME, PERMS, PARENT, &procfile_fops);

	if (proc_entry == NULL){
		return -ENOMEM;
	}
	return 0;
}

static void __exit timer_exit(void) {
	proc_remove(proc_entry);
}

module_init(timer_init);
module_exit(timer_exit);
