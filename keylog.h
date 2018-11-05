#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/debugfs.h>
#include <linux/ktime.h>
#include <linux/mutex.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/namei.h>
#include <linux/fs_struct.h>
#include <linux/miscdevice.h>
#include <linux/proc_fs.h>
#include <linux/interrupt.h>
#include <linux/ktime.h>
#include <linux/time.h>
#include <linux/ctype.h>
#include <asm/io.h>

#include <linux/seq_file.h>


static struct s_stroke {
	unsigned char		key;
	unsigned char		state;
	char			name[25];
	char			value;
	struct tm		time;
	struct list_head	stroke_lst;
};
