obj-m += hcsr04_sensor.o

KERNALDIR = /lib/modules/$(shell uname -r)/build

all:
	make -C $(KERNALDIR) M=$(PWD) modules
clean:
	make -C $(KERNALDIR) M=$(PWD) clean