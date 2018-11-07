#ifndef _KEYLOG_H_
#define _KEYLOG_H_
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/buffer_head.h>
#include <linux/ctype.h>
#include <linux/debugfs.h>
#include <linux/err.h>
#include <linux/fcntl.h>
#include <linux/file.h>
#include <linux/fs_struct.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kallsyms.h>
#include <linux/ktime.h>
#include <linux/mutex.h>
#include <linux/miscdevice.h>
#include <linux/namei.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/syscalls.h>
#include <linux/time.h>
#include <linux/uaccess.h>
#include <asm/io.h>
#include <asm/segment.h>
#include <asm/uaccess.h>

struct s_keyboard_map {
  int   key;
  int   ascii;
  char  *str;
  int   shift_ascii;
  char  *shift_str;
  bool  pressed;
  size_t  nb_pressed;
  size_t  nb_released;
};

struct s_stroke {
	unsigned char		key;
	unsigned char		state;
	const char		*name;
	char			value;
	struct tm		time;
	struct list_head	stroke_lst;
};

struct s_keyboard_map_lst {
  	int	key;
  	int	ascii;
  	char	*str;
  	size_t	nb_pressed;
  	size_t	nb_released;
	struct list_head	map_lst;
};

#endif
