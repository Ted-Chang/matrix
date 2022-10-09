# Makefile for fs module
MATRIX_ROOT_DIR := $(abspath $(CURDIR)/../..)
include $(MATRIX_ROOT_DIR)/kernel/Makefile.inc

C_SRCS = vfs.c fd.c initrd.c devfs.c procfs.c
LIB := $(call matrix_lib_list_to_static_libs,fs)

.PHONY: clean help

all: $(LIB)
	@:

clean:
	$(CLEAN_C) $(LIB)

$(LIB): $(OBJS)
	$(LIB_C)
