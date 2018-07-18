LDLIBS = $(shell pkg-config vips --libs)
CFLAGS = $(shell pkg-config vips --cflags) -ggdb3 -Wall -pedantic -std=c99

.DUMMY: default all clean test

default: all

get_region: main.o region.o llist.o
	$(CC) $(LDFLAGS) $(LDLIBS) -o $@ $^

test: region.test
	"./$<"

region.test: region.test.o region.o llist.o
	$(CC) $(LDFLAGS) $(LDLIBS) -o $@ $^

all: get_region

clean:
	$(RM) *.o get_region
