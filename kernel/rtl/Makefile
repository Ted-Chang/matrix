# Makefile for librt module
MATRIX_ROOT_DIR := $(abspath $(CURDIR)/../..)
include $(MATRIX_ROOT_DIR)/kernel/Makefile.inc

C_SRCS = avltree.c radixtree.c hashtable.c notifier.c object.c bitmap.c name.c kstrdup.c
LIB := $(call matrix_lib_list_to_static_libs,rtl)

.PHONY: clean help

all: $(LIB)
	@:

clean:
	$(CLEAN_C) $(LIB)

$(LIB): $(OBJS)
	$(LIB_C)
