# build/hc-sr04_driver: build/driver.o
# 	cc build/driver.o -o build/hc-sr04_driver

# build/driver.o: src/driver.c | build
# 	cc -c src/driver.c -o build/driver.o 

# build:
# 	mkdir -p $@

obj-m += driver.o

KERNALDIR = /lib/modules/$(shell uname -r)/build

all:
	make -C $(KERNALDIR) M=$(PWD) modules
clean:
	make -C $(KERNALDIR) M=$(PWD) clean