CC ?= gcc
CFLAGS ?= -Og -g3
LDFLAGS ?= 
BINNAME ?= extractjxr
OBJECTS = extractjxr.o mmap.o

all: $(BINNAME)

$(BINNAME): $(OBJECTS)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	rm -f $(BINNAME) $(OBJECTS)

