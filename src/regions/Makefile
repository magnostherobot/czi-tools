CC ?= cc
CFLAGS ?= -Og -ggdb3

LDLIBS += $(shell pkg-config vips --libs)
CFLAGS += $(shell pkg-config vips --cflags)

CFLAGS += -Wall -Werror -std=c99
TARGET = get_region
SRCS = $(wildcard *.c)
DEPS = ${SRCS:.c=.d}

TESTSRCS = $(wildcard *.test.c)
TESTS = ${TESTSRCS:.c=}

.PHONY: default all clean test

default: all

all: $(TARGET)

$(TARGET): main.o region.o llist.o name.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.c
	$(CC) -MMD -c $(CFLAGS) -o $@ $<
	sed -i'.bak' 's,\($*\.o\)[ :]*\(.*\),$@:$$\(wildcard \2\),g' $*.d
	$(RM) $*.d.bak

test: $(TESTS)
	for t in $^ ; do echo "$$t:" ; "./$$t" ; done

name.test: name.test.o name.o llist.o region.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

region.test: region.test.o region.o llist.o name.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

clean:
	$(RM) *.d *.o get_region *.test

-include $(DEPS)
