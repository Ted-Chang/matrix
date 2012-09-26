# Makefile for userspace utilities

TARGETS := $(patsubst %.c,../bin/%,$(wildcard *.c))


.PHONY: clean help

all: $(TARGETS)

$(TARGETS)%: %.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	$(RM) $(TARGETS)
