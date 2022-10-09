# Makefile for libc module
MATRIX_ROOT_DIR := $(abspath $(CURDIR)/../..)
include $(MATRIX_ROOT_DIR)/kernel/Makefile.inc

CSTDDIR=$(MATRIX_ROOT_DIR)/sdk/libc

C_SRCS = $(CSTDDIR)/stdio.c $(CSTDDIR)/string.c $(CSTDDIR)/div64.c $(CSTDDIR)/vsprintf.c $(CSTDDIR)/sprintf.c $(CSTDDIR)/format.c
LIB := $(call matrix_lib_list_to_static_libs,cstd)

.PHONY: clean help

all: $(LIB)
	@:

clean:
	$(CLEAN_C) $(LIB) -r $(CSTDDIR)/*.o

$(LIB): $(OBJS)
	$(LIB_C)
