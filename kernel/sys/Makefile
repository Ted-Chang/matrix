# Makefile for sys module
MATRIX_ROOT_DIR := $(abspath $(CURDIR)/../..)
include $(MATRIX_ROOT_DIR)/kernel/Makefile.inc

C_SRCS = timer.c syscall.c util.c mutex.c semaphore.c terminal.c unittest.c platform.c ioctx.c device.c acpi.c smp.c
AS_SRCS = acboot.s
LIB := $(call matrix_lib_list_to_static_libs,sys)

.PHONY: clean help

all: $(LIB)
	@:

clean:
	$(CLEAN_C) $(LIB)

$(LIB): $(OBJS)
	$(LIB_C)
