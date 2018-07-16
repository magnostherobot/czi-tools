LDLIBS = $(shell pkg-config vips --libs)
CFLAGS = $(shell pkg-config vips --cflags) -ggdb3

get_region: region.o llist.o
	$(CC) $(LDFLAGS) $(LDLIBS) -o $@ $^
