CC ?= /usr/bin/gcc
LD ?= ld
CFLAGS ?= -g3
LDFLAGS ?=
BINNAME ?= czinspect
LIBS ?=

CCSRC = $(wildcard src/*.c)
CCOBJ = ${CCSRC:.c=.o}
HEADERS = $(wildcard src/*.h) src/compat/compat.h
HELPSRC = $(wildcard helptxt/*.help)
HELPOBJ = ${HELPSRC:.help=.ho}
OBJECTS = $(CCOBJ) $(HELPOBJ)

CFLAGS += -std=c99 -Wall -Werror

ifeq ($(strip $(BSD)),)
CCSRC += $(wildcard src/compat/*.c)
CFLAGS += -DNOT_BSD
endif

GENDIR = includes
GENHEADERS = helptxt.h config.h

TOOLDIR = tools
TOOLOUT = gen

all: $(BINNAME)

$(CCSRC): $(HEADERS) $(GENHEADERS)

$(BINNAME): $(OBJECTS)
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)

$(GENDIR) $(TOOLOUT):
	mkdir $@

$(GENHEADERS): $(GENDIR) $(TOOLOUT)

%.ho: %.help
	$(LD) -r -b binary -o $@ $^

%.o: %.c
	$(CC) -c -o $@ $^ -I $(GENDIR) $(CFLAGS)

clean:
	rm -rf $(BINNAME) $(OBJECTS) $(GENDIR) $(TOOLOUT)


helptxt.h:
	echo '#ifndef _HELPTXT_H' >> $(GENDIR)/$@
	echo '#define _HELPTXT_H' >> $(GENDIR)/$@
	ls helptxt | tr '.' '_' | awk -f $(TOOLDIR)/genhelp-h.awk >> $(GENDIR)/$@
	echo '#endif /* _HELPTXT_H */' >> $(GENDIR)/$@

config.h: configtest
	$(TOOLOUT)/configtest > $(GENDIR)/$@

configtest: $(TOOLDIR)/configtest.c
	$(CC) -o $(TOOLOUT)/$@ $^ $(LDFLAGS)

test:
	# no-op for now

