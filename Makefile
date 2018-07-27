export CFLAGS = -O1

.PHONY: all default clean test

default: all

all:
	$(MAKE) -C src/extractjxr
	$(MAKE) -C src/regions
	$(MAKE) -C src/czinspect

clean:
	$(MAKE) clean -C src/extractjxr
	$(MAKE) clean -C src/regions
	$(MAKE) clean -C src/czinspect

test:
	$(MAKE) test -C src/extractjxr
	$(MAKE) test -C src/regions
	$(MAKE) test -C src/czinspect
