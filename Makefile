.PHONY: all default clean test

default: all

all:
	$(MAKE) -C src/extractjxr
	$(MAKE) -C src/regions

clean:
	$(MAKE) clean -C src/extractjxr
	$(MAKE) clean -C src/regions

test: default
	$(MAKE) test -C src/extractjxr
	$(MAKE) test -C src/regions
