CFLAGS = $(shell pkg-config vips --cflags --libs)

get_region: region.o llist.o
	$(CC) $(CFLAGS) $(LFLAGS) -o $@ $^
