#include <types.h>
#include <stddef.h>
#include "matrix/debug.h"

void print_test()
{
	kprintf("A String\n");
	kprintf("A hex:%x\n", 0x1000);
	kprintf("A decimal:%d\n", 0x1000);
	kprintf("A octal:%o\n", 0x1000);
	kprintf("A long long decimal:%ld\n", 0x100000000);
	kprintf("A long long hex:%lx\n", 0x100000000);
	kprintf("A String(%s)\n", "Hello, kprintf");
}
