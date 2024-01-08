obj-m += elevator.o

KDIR := /lib/modules/$(shell uname -r)/build

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	make -C $(KDIR) M=$(PWD) clean

insmod: all
	sudo rmmod $(elevator).ko
	sudo insmod $(elevator).ko

log:
	tail -f /var/log/messages
