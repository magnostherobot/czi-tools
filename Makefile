export CFLAGS = -O1

.PHONY: all default clean test bin

default: all

all:
	$(MAKE) -C src/regions
	$(MAKE) -C src/czinspect

clean:
	$(MAKE) clean -C src/regions
	$(MAKE) clean -C src/czinspect
	$(RM) $(PWD)/bin/get_region $(PWD)/bin/czinspect

test:
	$(MAKE) test -C src/regions
	$(MAKE) test -C src/czinspect

bin: all
	ln -s $(PWD)/src/regions/get_region  $(PWD)/bin/get_region
	ln -s $(PWD)/src/czinspect/czinspect $(PWD)/bin/czinspect
