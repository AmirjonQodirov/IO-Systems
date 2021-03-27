
obj-m += lab2.o

all:
		make -C /lib/modules/`uname -r`/build M=$(PWD) modules

clean:   
		make -C /lib/modules/`uname -r`/build M=$(PWD) clean

load:
		insmod lab2.ko

unload:
		rmmod lab2.ko



