# SPDX-License-Identifier: GPL-2.0-only

CC ?= gcc
CFLAGS ?= -O2
CFLAGS += -Wall -Wextra
LDLIBS += -lteec

PREFIX ?= /usr
BINDIR ?= $(PREFIX)/bin

all: amvdec-tee

amvdec-tee: amvdec-tee.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDLIBS)

install: amvdec-tee
	install -D -m 0755 amvdec-tee $(DESTDIR)$(BINDIR)/amvdec-tee

clean:
	rm -f amvdec-tee

.PHONY: all install clean
