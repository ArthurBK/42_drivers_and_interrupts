
obj-m += keylogs.o

keylogs-objs := misc_keylog.o misc_stats.o keylog.o irq_handler.o keymap.o file_op.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules 

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
