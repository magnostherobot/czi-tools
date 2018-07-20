CC ?= gcc
CFLAGS ?= -Og -g3 -std=c99 -D_POSIX_C_SOURCE=200809L
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

test:
	# No-op for now
