# Makefile for userspace utilities

TARGETS := $(patsubst %.c,../bin/%,$(wildcard *.c))

CFLAGS := -Wall -nostdlib -nostdinc -fno-builtin -fno-stack-protector


.PHONY: clean help

all: $(TARGETS)

$(TARGETS)%: %.c
	$(CC) $(CFLAGS) -m32 -g -I../loader -o $@ $<

clean:
	$(RM) $(TARGETS)
