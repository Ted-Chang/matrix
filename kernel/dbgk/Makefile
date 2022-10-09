# Makefile for debug module
MATRIX_ROOT_DIR := $(abspath $(CURDIR)/../..)
include $(MATRIX_ROOT_DIR)/kernel/Makefile.inc

C_SRCS = debug.c kd.c dbgheap.c symbol.c
LIB := $(call matrix_lib_list_to_static_libs,dbgk)

.PHONY: clean help

all: $(LIB)
	@:

clean:
	$(CLEAN_C) $(LIB)

$(LIB): $(OBJS)
	$(LIB_C)
