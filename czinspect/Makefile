CC ?= gcc
LD ?= ld
CFLAGS ?= -Og -ggdb3
LDFLAGS ?= 
BINNAME ?= czinspect
LIBS ?= 

CCSRC = czinspect.c 
CCOBJ = ${CCSRC:.c=.o}
HELPSRC = $(wildcard helptxt/*.help)
HELPOBJ = ${HELPSRC:.help=.ho}
OBJECTS = $(CCOBJ) $(HELPOBJ)
HEADERS = helptxt.h

all: $(BINNAME)

$(CCSRC): $(wildcard *.h) $(HEADERS)

$(BINNAME): $(OBJECTS)
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)

%.ho: %.help
	$(LD) -r -b binary -o $@ $^

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	rm -f $(BINNAME) $(OBJECTS) $(HEADERS)

helptxt.h:
	echo '#ifndef _HELPTXT_H' >> $@
	echo '#define _HELPTXT_H' >> $@
	ls helptxt | tr '.' '_' | awk -f genhelp.awk >> $@
	echo '#endif /* _HELPTXT_H */' >> $@

