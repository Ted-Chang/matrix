# Makefile for hal module
MATRIX_ROOT_DIR := $(abspath $(CURDIR)/../..)
include $(MATRIX_ROOT_DIR)/kernel/Makefile.inc

C_SRCS = core.c hal.c isr.c lapic.c spinlock.c
AS_SRCS = dscptr.s interrupt.s
LIB := $(call matrix_lib_list_to_static_libs,hal)

.PHONY: clean help

all: $(LIB)
	@:

clean:
	$(CLEAN_C) $(LIB)

$(LIB): $(OBJS)
	$(LIB_C)
