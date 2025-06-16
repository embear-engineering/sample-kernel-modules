obj-m += reset-reason.o

PWD := $(shell pwd)

KDIR ?= "/lib/modules/$(shell uname -r)/build"

.PHONY = check

all:
	make -C $(KDIR) M=$(PWD) modules

clean:
	make -C $(KDIR) M=$(PWD) clean

cfiles = $(obj-m:.o=.c)
check: $(cfiles)
	$(KDIR)/scripts/checkpatch.pl -f --max-line-length=100 $<
