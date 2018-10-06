CC ?= /usr/bin/gcc
LD ?= ld
CFLAGS ?= -Og -ggdb3
LDFLAGS ?=
BINNAME ?= czinspect
LIBS ?=

CCSRC = $(wildcard src/*.c)
CCOBJ = ${CCSRC:.c=.o}
HEADERS = $(wildcard src/*.h) src/compat/compat.h
OBJECTS = $(CCOBJ)

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

%.o: %.c
	$(CC) -c -o $@ $^ -I $(GENDIR) $(CFLAGS)

clean:
	rm -rf $(BINNAME) $(OBJECTS) $(GENDIR) $(TOOLOUT)


helptxt.h:
	echo '#ifndef _HELPTXT_H' >> $(GENDIR)/$@
	echo '#define _HELPTXT_H' >> $(GENDIR)/$@
	for i in helptxt/*; do \
		xxd -i $$i >> $(GENDIR)/$@ ; \
	done
	echo '#endif /* _HELPTXT_H */' >> $(GENDIR)/$@

config.h: configtest
	$(TOOLOUT)/configtest > $(GENDIR)/$@

configtest: $(TOOLDIR)/configtest.c
	$(CC) -o $(TOOLOUT)/$@ $^ $(LDFLAGS)

test:
	# no-op for now


