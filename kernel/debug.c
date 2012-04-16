#include <types.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include "hal/hal.h"
#include "hal/lirq.h"
#include "matrix/debug.h"
#include "matrix/global.h"
#include "proc/task.h"

uint32_t _debug_level = DL_DBG;

static uint16_t _cursor_x = 0;
static uint16_t _cursor_y = 0;
static uint16_t *_video_mem = (uint16_t *)0xB8000;

static void update_cursor()
{
	uint16_t cursor_pos = _cursor_y * 80 + _cursor_x;
	outportb(0x3D4, 14);
	outportb(0x3D5, cursor_pos >> 8);
	outportb(0x3D4, 15);
	outportb(0x3D5, cursor_pos);
}

static void scroll_scr()
{
	uint8_t attr_byte = (0 << 4) | (15 & 0x0F);
	uint16_t blank = 0x20 | (attr_byte << 8);

	if (_cursor_y >= 25) {
		int i;
		for (i = 0; i < 24 * 80; i++) {
			_video_mem[i] = _video_mem[i + 80];
		}

		for (i = 24 * 80; i < 25 * 80; i++) {
			_video_mem[i] = blank;
		}

		_cursor_y = 24;
	}
}

void putch(char ch)
{
	uint8_t background = 0;
	uint8_t foreground = 15;

	uint8_t attr_byte = (background << 4) | (foreground & 0x0F);

	uint16_t attr = attr_byte << 8;
	uint16_t *loc;

	if (ch == 0x08 && _cursor_x) {
		_cursor_x--;
	} else if (ch == 0x09) {
		_cursor_x = (_cursor_x + 8) & ~(8 - 1);
	} else if (ch == '\r') {
		_cursor_x = 0;
	} else if (ch == '\n') {
		_cursor_x = 0;
		_cursor_y++;
	} else if (ch >= ' ') {
		loc = _video_mem + (_cursor_y * 80 + _cursor_x);
		*loc = ch | attr;
		_cursor_x++;
	}

	if (_cursor_x >= 80) {
		_cursor_x = 0;
		_cursor_y++;
	}

	scroll_scr();
	update_cursor();
}

void putstr(const char *str)
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
		_video_mem[i] = blank;
	}

	_cursor_x = 0;
	_cursor_y = 0;
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
				/* Unsigned integers */
			case 'u': {
				int c = va_arg(args, int);
				char temp_str[32] = {0};
				itoa(c, 10, temp_str);
				putstr(temp_str);
				i++;
				break;
			}
				/* Display in hex */
			case 'X':
			case 'x': {
				int c = va_arg(args, int);
				char temp_str[32] = {0};
				itoa(c, 16, temp_str);
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

void panic(const char *message, const char *file, uint32_t line)
{
	irq_disable();
	kprintf("PANIC(%s) at %s:%d\n", message, file, line);
	for (; ; ) ;
}

void panic_assert(const char *file, uint32_t line, const char *desc)
{
	irq_disable();
	kprintf("ASSERTION FAILED(%s) at %s:%d\n", desc, file, line);
	for (; ; ) ;
}

#ifdef _DEBUG_SCHED

void check_runqueues(char *when)
{
	int q;
	struct task *tp;

	for (q = 0; q < NR_SCHED_QUEUES; q++) {
		if (_ready_head[q] && !_ready_tail[q]) {
			kprintf("head but no tail, priority(%d): %s\n", q, when);
			PANIC("Scheduling error!");
		}

		if (!_ready_head[q] && _ready_tail[q]) {
			kprintf("tail but no head, priority(%d): %s\n", q, when);
			PANIC("Scheduling error!");
		}

		if (_ready_tail[q] && _ready_tail[q]->next != NULL) {
			kprintf("tail and tail->next not null, priority(%d): %s\n",
				q, when);
			PANIC("Scheduling error!");
		}

		for (tp = _ready_head[q]; tp != NULL; tp = tp->next) {
			if (tp->priority != q) {
				kprintf("wrong priority: %s\n", when);
				PANIC("Scheduling error!");
			}
			if ((tp->next == NULL) && (_ready_tail[q] != tp)) {
				kprintf("last element not tail: %s\n", when);
				PANIC("Scheduling error!");
			}
		}
	}
}

#endif	/* _DEBUG_SCHED */
