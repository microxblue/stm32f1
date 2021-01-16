#ifndef _COMMON_H_
#define _COMMON_H_

#define  CMD_PACKET_LEN    12

typedef  unsigned char     BYTE;
typedef  unsigned short    WORD;
typedef  unsigned int     DWORD;

typedef  unsigned char    UINT8;
typedef  unsigned short  UINT16;
typedef  unsigned int    UINT32;

typedef  char              CHAR;
typedef  char              INT8;
typedef  short            INT16;
typedef  int              INT32;
typedef  unsigned int    size_t;

#ifndef __MODULE__
#define  putchar  PutChar
#define  getchar  GetChar
#define  haschar  HasChar
#define  printf   Printf
#endif

#ifndef __STRING__
void *memcpy(void *d, void *s, size_t n);
void *memset(void *s, int c, size_t n);
int   memcmp(void *s1, void *s2, size_t n);
int   strncmpi(void *s1, void *s2, size_t n);
char *strcpy(char *dest,const char *src);
char *strcat(char *dest, const char *src);
int   strcmp(const char * cs,const char * ct);
int   strncmp(const char * cs,const char * ct, int count);
int   strlen(const char * s);
#endif

char  toupper(char ch);
char  tolower(char ch);
char  getchar ();
void  putchar (char c);
char  haschar ();

char *skipchar (char *str, char ch);
char *findchar (char *str, char ch);
unsigned long xtoi (char *str);

void  Printf   (const char *fmt, ...);
void  Puts(const char *s);

BYTE  SetCon (BYTE value);

#endif
