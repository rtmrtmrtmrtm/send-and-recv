obj-m+=sar.o

all: tst
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) clean
	rm -f tst

tst : tst.cc
	c++ -O -std=c++11 -o tst tst.cc -lpthread
