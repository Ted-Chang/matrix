# Make file for tools
CFLAGS := -Wall

TARGET := make_initrd

all: $(TARGET)
	@echo "Making os image tools"

$(TARGET):
	gcc -g -o make_initrd make_initrd.c

clean:
	$(RM) -f $(TARGET)

help:
	@echo "Available make targets:"
	@echo
	@echo "all	- build os image tools"
	@echo "clean	- remove object files and binaries"
	@echo "help	- print this help"
