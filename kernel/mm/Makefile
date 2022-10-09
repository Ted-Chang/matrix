# Makefile for mm module
MATRIX_ROOT_DIR := $(abspath $(CURDIR)/../..)
include $(MATRIX_ROOT_DIR)/kernel/Makefile.inc

C_SRCS = vector.c mmu.c kmem.c page.c malloc.c slab.c phys.c va.c
LIB := $(call matrix_lib_list_to_static_libs,mm)

.PHONY: clean help

all: $(LIB)
	@:

clean:
	$(CLEAN_C) $(LIB)

$(LIB): $(OBJS)
	$(LIB_C)
