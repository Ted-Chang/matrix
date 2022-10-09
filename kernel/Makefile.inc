# Makefile.inc 
# Define the global variable for all submodules here

ifneq ($(V),1)
Q ?= @
endif
S ?= $(notdir $(CURDIR))

ifeq ($(MAKECMDGOALS),)
MAKECMDGOALS=$(.DEFAULT_GOAL)
endif

# CFLAGS for kernel
COMMON_CFLAGS = -g -Wall -Wextra -nostdlib -nostdinc -fno-builtin -fno-stack-protector -D__KERNEL__ -D_X86_
COMMON_CFLAGS += -I$(MATRIX_ROOT_DIR)/kernel/include
COMMON_CFLAGS += -I$(MATRIX_ROOT_DIR)/sdk/include

COMMON_CFLAGS += -DBITS_PER_LONG=32 -m32

ifeq ($(CONFIG_WERROR),y)
COMMON_CFLAGS += -Werror
endif

ifeq ($(CONFIG_DEBUG),y)
COMMON_CFLAGS += -DDEBUG -O0 -fno-omit-frame-pointer
else
COMMON_CFLAGS += -DNDEBUG -O2
endif

CFLAGS += $(COMMON_CFLAGS) -Wno-pointer-sign -std=gnu99
CXXFLAGS += $(COMMON_CFLAGS) -std=c++0x

MAKEFLAGS += --no-print-directory

C_SRCS += $(C_SRCS-y)
CXX_SRCS += $(CXX_SRCS-y)
AS_SRCS += $(AS_SRCS-y)

OBJS = $(C_SRCS:.c=.o) $(CXX_SRCS:.cpp=.o) $(AS_SRCS:.s=.o)

DEPFLAGS = -MMD -MP -MF $*.d.tmp

# Global ASFLAGS
ASFLAGS := -felf
AS=nasm

COMPILE_C=\
	$(Q)echo "   CC $S/$@"; \
	$(CC) -o $@ $(DEPFLAGS) $(CFLAGS) -c $< && \
	mv -f $*.d.tmp $*.d && touch -c $@

COMPILE_CXX=\
	$(Q)echo "   CC $S/$@"; \
	$(CC) -o $@ $(DEPFLAGS) $(CFLAGS) -c $< && \
	mv -f $*.d.tmp $*.d && touch -c $@

COMPILE_AS=\
	$(Q)echo "   AS $@/$@"; \
	nasm $(ASFLAGS) -o $@ $<

LINK_C=\
	$(Q)echo " LINK $(notdir $@)"; \
	$(CC) -o $@ $(CPPFLAGS) $(LDFLAGS) $(OBJS) $(LIBS)

LIB_C=\
	$(Q)echo "  LIB $(notdir $@)"; \
	rm -f $@; \
	mkdir -p $(dir $@); \
	$(AR) crDs $@ $(OBJS)

CLEAN_C=\
	$(Q)rm -f *.a *.o *.d *.d.tmp *.gcno *.gcda

%.o: %.c %.d $(MAKEFILE_LIST)
	$(COMPILE_C)

%.o: %.cpp %.d $(MAKEFILE_LIST)
	$(COMPILE_CXX)

%.o: %.s %.d $(MAKEFILE_LIST)
	$(COMPILE_AS)

%.d: ;

define matrix_lib_list_to_static_libs
$(1:%=$(MATRIX_ROOT_DIR)/bin/lib/libmtx_%.a)
endef

MATRIX_LIB_FILES = $(call matrix_lib_list_to_static_libs,$(MATRIX_LIB_LIST))
MATRIX_LIB_LINKER_ARGS = \
	-L$(MATRIX_ROOT_DIR)/bin/lib \
	$(MATRIX_LIB_LIST:%=-lmtx_%)