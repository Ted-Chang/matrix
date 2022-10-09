# Makefile for proc module
MATRIX_ROOT_DIR := $(abspath $(CURDIR)/../..)
include $(MATRIX_ROOT_DIR)/kernel/Makefile.inc

C_SRCS = process.c sched.c thread.c signal.c
AS_SRCS = switch.s
LIB := $(call matrix_lib_list_to_static_libs,proc)

.PHONY: clean help

all: $(LIB)
	@:

clean:
	$(CLEAN_C) $(LIB)

$(LIB): $(OBJS)
	$(LIB_C)
