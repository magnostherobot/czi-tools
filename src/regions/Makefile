CC ?= cc
CFLAGS ?= -Og -ggdb3

LDLIBS += $(shell pkg-config vips --libs)
CFLAGS += $(shell pkg-config vips --cflags)

CFLAGS += -Wall -Werror -std=c99
TARGET = get_region
SRCS = $(wildcard *.c)

TESTSRCS = $(wildcard *.test.c)
TESTS = ${TESTSRCS:.c=}

.PHONY: default all clean test

default: all

all: $(TARGET)

$(TARGET): main.o region.o llist.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.c
	$(CC) -MMD -c $(CFLAGS) -o $@ $<
	sed -i 's,\($*\.o\)[ :]*\(.*\), $@ : $$\(wildcard \2\)\n\1 : \2,g' $*.d

test: $(TESTS)
	for t in $^ ; do "./$$t" ; done

region.test: region.test.o region.o llist.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

clean:
	$(RM) *.d *.o get_region *.test

-include ${SRCS:.c=.d}
