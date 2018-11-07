#include "keylog.h"

extern struct s_keyboard_map keyboard_mapping[];
LIST_HEAD(head_keymap_lst);
DEFINE_SPINLOCK(mr_lock);

int cmp_pressed(void *priv, struct list_head *a, struct list_head *b)
{
	struct s_keyboard_map_lst		*a_stroke = NULL;
	struct s_keyboard_map_lst		*b_stroke = NULL;

	a_stroke = list_entry(a, struct s_keyboard_map_lst, map_lst);
	b_stroke = list_entry(b, struct s_keyboard_map_lst, map_lst);
	return a_stroke->nb_pressed < b_stroke->nb_pressed ? 1 : 0;
}

int cmp_released(void *priv, struct list_head *a, struct list_head *b)
{
	struct s_keyboard_map_lst		*a_stroke = NULL;
	struct s_keyboard_map_lst		*b_stroke = NULL;

	a_stroke = list_entry(a, struct s_keyboard_map_lst, map_lst);
	b_stroke = list_entry(b, struct s_keyboard_map_lst, map_lst);
	return a_stroke->nb_released < b_stroke->nb_released ? 1 : 0;
}

int stats_show(struct seq_file *seq_file, void *p)
{
	struct s_keyboard_map	entry;
	struct s_keyboard_map_lst	*keymap_iter = NULL;
	struct s_keyboard_map_lst	*keymap_elem = NULL;
	int i;
	int j;

	j = 0;
	for (i = 0; i < MAX_KEYS; ++i) {
		keymap_elem = kmalloc(sizeof(struct s_keyboard_map_lst ), GFP_ATOMIC);
		entry = keyboard_mapping[i];
		keymap_elem->key = entry.key;
		keymap_elem->ascii = entry.ascii;
		keymap_elem->str = entry.str;
		keymap_elem->nb_pressed = entry.nb_pressed;
		keymap_elem->nb_released = entry.nb_released;
		list_add(&(keymap_elem->map_lst), &head_keymap_lst);
	}
	list_sort(NULL, &head_keymap_lst, cmp_pressed);
	seq_printf(seq_file, "TOP 3 PRESSED KEYS\n");
	list_for_each_entry(keymap_iter, &head_keymap_lst, map_lst)
	{
		if (keymap_iter->nb_pressed != 0 && j < 3) {
			seq_printf(seq_file, "%s (%d) - %li times\n",
				   keymap_iter->str,
				   keymap_iter->key,
			   	   keymap_iter->nb_pressed);
			++j;
		}
	}
	j = 0;
	seq_printf(seq_file, "TOP 3 RELEASED KEYS\n");
	list_sort(NULL, &head_keymap_lst, cmp_released);
	list_for_each_entry(keymap_iter, &head_keymap_lst, map_lst)
	{
		if (keymap_iter->nb_pressed != 0 && j < 3) {
			seq_printf(seq_file, "%s (%d) - %li times\n",
				   keymap_iter->str,
				   keymap_iter->key,
			   	   keymap_iter->nb_released);
			++j;
		}
		kfree(keymap_iter);
	}
	return 0;
}

static int stats_open(struct inode *inode, struct file *file)
{
	int ret;

	spin_lock(&mr_lock);
	file->private_data = NULL;
	ret = single_open(file, &stats_show, NULL);
	spin_unlock(&mr_lock);
	return ret;
}

static ssize_t	stats_write(struct file *file, const char __user *buf,
			  size_t size, loff_t *offset)
{
	return 0;
}

static ssize_t stats_read(struct file *file, char __user *buf, size_t size,
			 loff_t *offset)
{
	int	ret;

	spin_lock(&mr_lock);
	ret = seq_read(file, buf, size, offset);
	spin_unlock(&mr_lock);
	return ret;
}

static int stats_release(struct inode *inode, struct file *file)
{
	int	ret;

	spin_lock(&mr_lock);
	ret = single_release(inode, file);
	spin_unlock(&mr_lock);
	return ret;
}

static struct file_operations const stats_file_fops = {
	.owner =	THIS_MODULE,
	.open =		stats_open,
	.write =	stats_write,
	.read =		stats_read,
	.release =	stats_release,
	.llseek =	seq_lseek,
};

struct miscdevice		stats_dev = {
	MISC_DYNAMIC_MINOR,
	MODULE_STATS_NAME,
	&stats_file_fops
};

EXPORT_SYMBOL(head_keymap_lst);
EXPORT_SYMBOL(stats_dev);
