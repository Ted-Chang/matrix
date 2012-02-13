#include "types.h"
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include "util.h"

static uint16_t cursor_x = 0;
static uint16_t cursor_y = 0;
static uint16_t *video_mem = (uint16_t *)0xB8000;

static void update_cursor()
{
	uint16_t cursor_pos = cursor_y * 80 + cursor_x;
	outportb(0x3D4, 14);
	outportb(0x3D5, cursor_pos >> 8);
	outportb(0x3D4, 15);
	outportb(0x3D5, cursor_pos);
}

static void scroll_scr()
{
	uint8_t attr_byte = (0 << 4) | (15 & 0x0F);
	uint16_t blank = 0x20 | (attr_byte << 8);

	if (cursor_y >= 25) {
		int i;
		for (i = 0; i < 24 * 80; i++) {
			video_mem[i] = video_mem[i + 80];
		}

		for (i = 24 * 80; i < 25 * 80; i++) {
			video_mem[i] = blank;
		}

		cursor_y = 24;
	}
}

void putch(char ch)
{
	uint8_t background = 0;
	uint8_t foreground = 15;

	uint8_t attr_byte = (background << 4) | (foreground & 0x0F);

	uint16_t attr = attr_byte << 8;
	uint16_t *loc;

	if (ch == 0x08 && cursor_x) {
		cursor_x--;
	} else if (ch == 0x09) {
		cursor_x = (cursor_x + 8) & ~(8 - 1);
	} else if (ch == '\r') {
		cursor_x = 0;
	} else if (ch == '\n') {
		cursor_x = 0;
		cursor_y++;
	} else if (ch >= ' ') {
		loc = video_mem + (cursor_y * 80 + cursor_x);
		*loc = ch | attr;
		cursor_x++;
	}

	if (cursor_x >= 80) {
		cursor_x = 0;
		cursor_y++;
	}

	scroll_scr();
	update_cursor();
}

void putstr(char *str)
{
	int i;
	if (!str)
		return;
	i = 0;
	while (str[i]) {
		putch(str[i++]);
	}
}

void clear_scr()
{
	uint8_t attr_byte = (0 << 4) | (15 & 0x0F);
	uint16_t blank = 0x20 | (attr_byte << 8);
	int i;
	for (i = 0; i < 80 * 25; i++) {
		video_mem[i] = blank;
	}

	cursor_x = 0;
	cursor_y = 0;
	update_cursor();
}

int kprintf(const char *str, ...)
{
	size_t i;
	va_list args;

	if (!str)
		return 0;

	va_start(args, str);

	for (i = 0; i < strlen(str); i++) {
		switch (str[i]) {
		case '%':
			switch (str[i+1]) {
			/* Characters */
			case 'c': {
				char c = va_arg(args, char);
				putch(c);
				i++;
				break;
			}
			/* Address of the string */
			case 's': {
				int c = (int)va_arg(args, int);
				char temp_str[64] = {0};
				strcpy(temp_str, (const char *)c);
				putstr(temp_str);
				i++;
				break;
			}
			/* Integers */
			case 'd':
			case 'i': {
				int c = va_arg(args, int);
				char temp_str[32] = {0};
				itoa_s(c, 10, temp_str);
				putstr(temp_str);
				i++;
				break;
			}
			/* Display in hex */
			case 'X':
			case 'x': {
				int c = va_arg(args, int);
				char temp_str[32] = {0};
				itoa_s(c, 16, temp_str);
				putstr(temp_str);
				i++;
				break;
			}
			default:
				va_end(args);
				return 1;
			}
			break;
		default:
			putch(str[i]);
			break;
		}
	}

	va_end(args);
	return 1;
}


void panic(const char message, const char *file, uint32_t line)
{
	asm volatile("cli");	// Disable all interrupts
	kprintf("PANIC(%s) at %s:%d", message, file, line);
	for (; ; ) ;
}
