#include <stdarg.h>
#include "Common.h"

#define putc  PutChar

#define out(c)      *bf++ = c

void Printf (const char *fmt, ...)
{
	va_list      va;
	char         ch;
	unsigned int mask;
	unsigned int num;
	char         buf[12];
	char        *bf;
	char        *p;
	char         uc;
	char         zs;

	va_start (va, fmt);

	while ((ch = *(fmt++))) {
		if (ch != '%') {
			putc (ch);
		} else {
			char lz = 0;
			char w = 1;
			char hx = 0;
			unsigned char dgt;
			ch = *(fmt++);
			if (ch == '0') {
				ch = *(fmt++);
				lz = 1;
			}
			if (ch >= '0' && ch <= '9') {
				w = 0;
				while (ch >= '0' && ch <= '9') {
					w = (((w << 2) + w) << 1) + ch - '0';
					ch = *fmt++;
				}
			}
			bf = buf;
			p = bf;
			zs = 0;
			switch (ch) {
			case 0:
				goto abort;

			case 'x':
			case 'X' :
				hx = 1;
			case 'u':
			case 'd' :
				num = va_arg (va, unsigned int);
				uc  = (ch == 'X') ? 'A' : 'a';
				if (ch == 'd' && (int)num < 0) {
					num = -(int)num;
					out ('-');
				}
				if (hx) {
					mask = 0x10000000;
				} else {
					mask = 1000000000;
				}
				while (mask) {
					dgt = 0;
					while (num >= mask) {
						num -= mask;
						dgt++;
					}
					if (zs || dgt > 0) {
						out (dgt + (dgt < 10 ? '0' : uc - 10));
						zs = 1;
					}
					mask = hx ? mask >> 4 : mask / 10;
				}
				if (zs == 0) {
					out ('0');
				}
				break;
			case 'c' :
				out ((char)(va_arg (va, int)));
				break;
			case 's' :
				p = va_arg (va, char *);
				break;
			case '%' :
				out ('%');
			default:
				break;
			}
			*bf = 0;
			bf = p;
			while (*bf++ && w > 0) {
				w--;
			}
			while (w-- > 0) {
				putc (lz ? '0' : ' ');
			}
			while ((ch = *p++)) {
				putc (ch);
			}
		}
	}
abort:
	;
	va_end (va);
}

