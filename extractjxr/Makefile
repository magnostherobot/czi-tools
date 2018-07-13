CC ?= gcc
CFLAGS ?= -Og -g3
LDFLAGS ?= 
BINNAME ?= extractjxr
OBJECTS = extractjxr.o mmap.o zeiss.o json.o
LIBS ?= 

all: $(BINNAME)

$(BINNAME): $(OBJECTS)
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	rm -f $(BINNAME) $(OBJECTS)

