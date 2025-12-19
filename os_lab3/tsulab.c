#include<linux/init.h>
#include<linux/kernel.h>
#include<linux/module.h>
#include<linux/proc_fs.h>
#include<linux/seq_file.h>
#include<linux/slab.h>
#include<linux/time.h>
#include<linux/rtc.h>

MODULE_LICENSE("GPL");

#define PROC_FILE_NAME "tsulab"
#define MESSAGE_WELCOME "Welcome to the Tomsk State University"
#define MESSAGE_FAREWELL "Tomsk State University forever!"

static struct proc_dir_entry* tsulab_proc_file = NULL;

static int calculate_minutes_to_new_year(void) {
    struct timespec64 now;
    struct rtc_time tm_now, tm_new_year;
    struct timespec64 ts_new_year;
    s64 seconds_diff;
    int minutes;

    ktime_get_real_ts64(&now);
    rtc_time64_to_tm(now.tv_sec, &tm_now);

    tm_new_year.tm_year = tm_now.tm_year + 1;
    tm_new_year.tm_mon = 0;
    tm_new_year.tm_mday = 1;
    tm_new_year.tm_hour = 0;
    tm_new_year.tm_min = 0;
    tm_new_year.tm_sec = 0;
    tm_new_year.tm_wday = 0;
    tm_new_year.tm_yday = 0;
    tm_new_year.tm_isdst = 0;

    ts_new_year.tv_sec = rtc_tm_to_time64(&tm_new_year);
    ts_new_year.tv_nsec = 0;

    seconds_diff = ts_new_year.tv_sec - now.tv_sec;

    if (seconds_diff < 0) {
        tm_new_year.tm_year += 1;
        ts_new_year.tv_sec = rtc_tm_to_time64(&tm_new_year);
        seconds_diff = ts_new_year.tv_sec - now.tv_sec;
    }
    minutes = (int)((seconds_diff + 59) / 60);
    return minutes;
}
static int tsulab_proc_show(struct seq_file* m, void* v) {
    int minutes_left;
    minutes_left = calculate_minutes_to_new_year();
    seq_printf(m, "Minutes until New Year: %d\n", minutes_left);
    return 0;
}
static int tsulab_proc_open(struct inode* inode, struct file* file) {
    return single_open(file, tsulab_proc_show, NULL);
}
static const struct proc_ops tsulab_proc_fops = {
    .proc_open = tsulab_proc_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};
static int __init module_init_function(void) {
    printk(KERN_INFO "%s\n", MESSAGE_WELCOME);
    tsulab_proc_file = proc_create(PROC_FILE_NAME, 0644, NULL, &tsulab_proc_fops);
    if (!tsulab_proc_file) {
        printk(KERN_ERR "Failed to create /proc/%s\n", PROC_FILE_NAME);
        return -ENOMEM;
    }
    printk(KERN_INFO "Created /proc/%s\n", PROC_FILE_NAME);
    printk(KERN_INFO "Module loaded successfully\n");
    return 0;
}
static void __exit module_exit_function(void) {
    if (tsulab_proc_file) {
        proc_remove(tsulab_proc_file);
        printk(KERN_INFO "Removed /proc/%s\n", PROC_FILE_NAME);
    }
    printk(KERN_INFO "%s\n", MESSAGE_FAREWELL);
}

module_init(module_init_function);
module_exit(module_exit_function);

