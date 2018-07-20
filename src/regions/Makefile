MD ?= $(CC) -MM

LDLIBS = $(shell pkg-config vips --libs)
CFLAGS = $(shell pkg-config vips --cflags) -ggdb3 -Wall -pedantic -std=c99
TARGET = get_region
SRCS = $(wildcard *.c)

TESTSRCS = $(wildcard *.test.c)
TESTS = ${TESTSRCS:.c=}

.DUMMY: default all clean test

default: all

all: $(TARGET)

$(TARGET): main.o region.o llist.o
	$(CC) $(LDFLAGS) $(LDLIBS) -o $@ $^

%.o: %.c
	$(CC) -MMD -c $(CFLAGS) -o $@ $<
	sed -i 's,\($*\.o\)[ :]*\(.*\), $@ : $$\(wildcard \2\)\n\1 : \2,g' $*.d

test: $(TESTS)
	for t in $^ ; do "./$$t" ; done

region.test: region.test.o region.o llist.o
	$(CC) $(LDFLAGS) $(LDLIBS) -o $@ $^

clean:
	$(RM) *.d *.o get_region *.test

-include ${SRCS:.c=.d}
