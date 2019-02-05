obj-m := traverseAncestry.o
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
	gcc procAncestry.c -o procAncestry
	gcc ./test/testFile.c -o ./test/testFile

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	-rm procAncestry
	-rm ./test/testFile
