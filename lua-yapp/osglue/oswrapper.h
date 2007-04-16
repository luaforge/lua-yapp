#ifndef OSWRAPPER_H
#define OSWRAPPER_H

/*
 * declare what the regular PalmOS headers are missing
 */

#include <stddef.h>
#include <stdio.h>

#include "codesections.h"

/* stdlib.h */
void exit(int status) CSEC_RTL;
int rand(void) CSEC_RTL;
void srand(unsigned int seed) CSEC_RTL;
int system(const char *command) CSEC_RTL;
char *getenv(const char *name) CSEC_RTL;
void *malloc(const size_t size) CSEC_RTL;
void free(void *p) CSEC_RTL;
void *realloc(void *ptr, size_t size) CSEC_RTL;
unsigned long strtoul(const char *s, char **end, int base) CSEC_RTL;
long strtol(const char *s, char **end, int base) CSEC_RTL;
double strtod(const char *nptr, char **endptr) CSEC_RTL;

void exit(int status);
int rand(void);
void srand(unsigned int seed);
int system(const char *command);
char *getenv(const char *name);
void *malloc(const size_t size);
void free(void *p);
void *realloc(void *ptr, size_t size);
unsigned long strtoul(const char *s, char **end, int base);
long strtol(const char *s, char **end, int base);
double strtod(const char *nptr, char **endptr);
enum {
	EXIT_SUCCESS = 0,
	EXIT_FAILURE,
};
#define RAND_MAX 32768

#if ! defined LONG_MIN
# define LONG_MIN 0x80000000L
#endif
#if ! defined LONG_MAX
# define LONG_MAX 0x7fffffffL
#endif

/* errno codes, shall we introduce an <errno.h> file? */
enum {
	ERANGE = 1,
	EINVAL,
};

/* locale.h */
struct lconv *localeconv(void) CSEC_RTL;
char *setlocale(int category, const char *locale) CSEC_RTL;

struct lconv {
	char *decimal_point;
};
enum {
	LC_ALL, LC_COLLATE, LC_CTYPE,
	LC_MONETARY, LC_NUMERIC, LC_TIME,
};
struct lconv *localeconv(void);
char *setlocale(int category, const char *locale);

/* string.h */
int strcoll(const char *s1, const char *s2) CSEC_RTL;
int strcmp(const char *s1, const char *s2) CSEC_RTL;
char *strerror(int errnum) CSEC_RTL;
size_t strlen(const char *s) CSEC_RTL;
char *strchr(const char *s, int c) CSEC_RTL;

int strcoll(const char *s1, const char *s2);
int strcmp(const char *s1, const char *s2);
char *strerror(int errnum);
size_t strlen(const char *s);
char *strchr(const char *s, int c);

/* stdio.h */
/* used in lauxlib.c, lbaselib.c, ldblib.c, liolib.c */
struct file_type;
typedef struct file_type FILE;

FILE *fopen(const char *path, const char *mode) CSEC_RTL;
int fclose(FILE *fp) CSEC_RTL;
int fprintf(FILE *stream, const char *format, ...) CSEC_RTL;
int printf(const char *format, ...) CSEC_RTL;
int fscanf(FILE *stream, const char *format, ...) CSEC_RTL;
int scanf(const char *format, ...) CSEC_RTL;
int sscanf(const char *s, const char *format, ...) CSEC_RTL;
int fputs(const char *s, FILE *stream) CSEC_RTL;
char *fgets(char *s, int size, FILE *stream) CSEC_RTL;
int fgetc(FILE *stream) CSEC_RTL;
int getc(FILE *stream) CSEC_RTL;
int ungetc(int c, FILE *stream) CSEC_RTL;
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream) CSEC_RTL;
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream) CSEC_RTL;
int feof(FILE *stream) CSEC_RTL;
int ferror(FILE *stream) CSEC_RTL;
void clearerr(FILE *stream) CSEC_RTL;
FILE *tmpfile (void) CSEC_RTL;
int fseek(FILE *stream, long offset, int whence) CSEC_RTL;
long ftell(FILE *stream) CSEC_RTL;
int setvbuf(FILE *stream, char *buf, int mode , size_t size) CSEC_RTL;
int fflush(FILE *stream) CSEC_RTL;
int remove(const char *pathname) CSEC_RTL;
int rename(const char *oldpath, const char *newpath) CSEC_RTL;
char *tmpnam(char *s) CSEC_RTL;
int snprintf(char *str, const size_t size, const char *format, ...) CSEC_RTL;
int sprintf(char *str, const char *format, ...) CSEC_RTL;
extern FILE *get_stdio_stream(const int n) CSEC_RTL;

FILE *fopen(const char *path, const char *mode);
int fclose(FILE *fp);
int fprintf(FILE *stream, const char *format, ...);
int printf(const char *format, ...);
int fscanf(FILE *stream, const char *format, ...);
int scanf(const char *format, ...);
int sscanf(const char *s, const char *format, ...);
int fputs(const char *s, FILE *stream);
char *fgets(char *s, int size, FILE *stream);
int fgetc(FILE *stream);
int getc(FILE *stream);
int ungetc(int c, FILE *stream);
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
int feof(FILE *stream);
int ferror(FILE *stream);
void clearerr(FILE *stream);
FILE *tmpfile (void);
int fseek(FILE *stream, long offset, int whence);
long ftell(FILE *stream);
int setvbuf(FILE *stream, char *buf, int mode , size_t size);
int fflush(FILE *stream);
int remove(const char *pathname);
int rename(const char *oldpath, const char *newpath);
char *tmpnam(char *s);
int snprintf(char *str, const size_t size, const char *format, ...);
int sprintf(char *str, const char *format, ...);

#undef stdin
#undef stdout
#undef stderr
extern FILE *get_stdio_stream(const int n);
#define stdin  get_stdio_stream(0)
#define stdout get_stdio_stream(1)
#define stderr get_stdio_stream(2)

#ifndef EOF
  #define EOF -1
#endif

enum {
	SEEK_SET,
	SEEK_CUR,
	SEEK_END,
};
enum {
	_IONBF,
	_IOFBF,
	_IOLBF,
};
#define L_tmpnam 32
#define BUFSIZ 2048	/* is 8K in Linux */

/* errno.h */
extern int errno_(void) CSEC_RTL;
extern int errno_(void); /* should be removed, used for FILE* stuff only (I hope) */
#define errno errno_()

/* time.h */
typedef long time_t;
typedef long clock_t;
struct tm {
	int tm_sec;
	int tm_min;
	int tm_hour;
	int tm_mday;
	int tm_mon;
	int tm_year;
	int tm_wday;
	int tm_yday;
	int tm_isdst;
};

clock_t clock(void) CSEC_RTL;
struct tm *gmtime(const time_t *timep) CSEC_RTL;
struct tm *localtime(const time_t *timep) CSEC_RTL;
size_t strftime(char *s, size_t max, const char *format, const struct tm *tm) CSEC_RTL;
time_t time(time_t *t) CSEC_RTL;
time_t mktime(struct tm *tm) CSEC_RTL;
time_t difftime(time_t time1, time_t time0) CSEC_RTL;

clock_t clock(void);
struct tm *gmtime(const time_t *timep);
struct tm *localtime(const time_t *timep);
size_t strftime(char *s, size_t max, const char *format, const struct tm *tm);
time_t time(time_t *t);
time_t mktime(struct tm *tm);
time_t difftime(time_t time1, time_t time0);
#define CLOCKS_PER_SEC 20 /* XXX TODO */

/* signal.h */
/* used in lua.c */
typedef void (*sighandler_t)(int);

sighandler_t signal(int signum, sighandler_t handler) CSEC_RTL;

sighandler_t signal(int signum, sighandler_t handler);
#define SIG_ERR (void *)(-1)
#define SIG_DFL (void *)0
enum {
	SIGINT = 2,
};

/* ctype.h needed? used in llex.c */

/* assert.h */
/* referenced when the LUA_USE_APICHECK define is set */
int assert_int(int cond, const char *txt) CSEC_RTL;
#define assert(cond) assert_int(cond, #cond)

#endif /* OSWRAPPER_H */
