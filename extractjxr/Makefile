CC ?= gcc
CFLAGS ?= -Og -g3
LDFLAGS ?= 
BINNAME ?= extractjxr
SOURCES = extractjxr.c mmap.c zeiss.c json.c compat/reallocarray.c compat/strtonum.c
OBJECTS = ${SOURCES:.c=.o}
LIBS ?= 

all: $(BINNAME)

$(BINNAME): $(OBJECTS)
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	rm -f $(BINNAME) $(OBJECTS)

