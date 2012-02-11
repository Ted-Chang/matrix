/*
 * main.c
 */

int main(struct multiboot *mboot_ptr)
{
	putstr("Hello world!\n");
	return 0xDEADBABA;
}
