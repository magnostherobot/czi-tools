export CFLAGS = -O1

.PHONY: all default clean test bin

default: all

all:
	$(MAKE) -C src/regions
	$(MAKE) -C src/czinspect

clean:
	$(MAKE) clean -C src/regions
	$(MAKE) clean -C src/czinspect
	$(RM) ./bin/get_region ./bin/czinspect

test:
	$(MAKE) test -C src/regions
	$(MAKE) test -C src/czinspect

bin: all
	ln -s ./src/regions/get_region  ./bin/get_region
	ln -s ./src/czinspect/czinspect ./bin/czinspect
