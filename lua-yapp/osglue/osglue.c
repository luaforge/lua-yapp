/* ----- osglue.c -------------------------------------------- */
/*
 * implement the part of the C runtime which PalmOS is lacking
 */

#include <PalmOS.h>
#include "../contrib/MathLib/MathLib.h"

#include "oswrapper.h"
#include "osglueinject.h"
#include "palmappinject.h"

#include <string.h>
#include <ctype.h>
#include "math.h"


struct format_desc;

static void dbg(void) CSEC_RTL;
static UInt32 PMrand(void) CSEC_RTL;
static void fopen_file(const char *path, struct file_type *f) CSEC_RTL;
static void fopen_memo(const char *path, struct file_type *f) CSEC_RTL;
static void fopen_fsdb(const char *path, struct file_type *f) CSEC_RTL;
static int fclose_file(struct file_type *f) CSEC_RTL;
static int fclose_memo(struct file_type *f) CSEC_RTL;
static int fclose_fsdb(struct file_type *f) CSEC_RTL;
static int formatted_write(const char *fmt, va_list args, char **s, int *l) CSEC_RTL;
static size_t fwrite_file(const void *ptr, size_t size, size_t nmemb, FILE *f) CSEC_RTL;
static size_t fwrite_memo(const void *ptr, size_t size, size_t nmemb, FILE *f) CSEC_RTL;
static size_t fwrite_fsdb(const void *ptr, size_t size, size_t nmemb, FILE *f) CSEC_RTL;
static char *fread_stdin(char *s, int *size, FILE *stream, const int term, const int delim) CSEC_RTL;
static char *fread_file(char *s, int size, FILE *f, const int term, const int delim) CSEC_RTL;
static char *fread_memo(char *s, int size, FILE *f, const int term, const int delim) CSEC_RTL;
static char *fread_fsdb(char *s, int size, FILE *f, const int term, const int delim) CSEC_RTL;
int fseek_file(FILE *stream, long offset, int whence) CSEC_RTL;
int fseek_memo(FILE *stream, long offset, int whence) CSEC_RTL;
int fseek_fsdb(FILE *stream, long offset, int whence) CSEC_RTL;
long ftell_file(FILE *stream) CSEC_RTL;
long ftell_memo(FILE *stream) CSEC_RTL;
long ftell_fsdb(FILE *stream) CSEC_RTL;
int fflush_file(FILE *f) CSEC_RTL;
int fflush_memo(FILE *f) CSEC_RTL;
int fflush_fsdb(FILE *f) CSEC_RTL;
static int remove_file(const char *pathname, const char *filename) CSEC_RTL;
static int remove_memo(const char *pathname, const char *filename) CSEC_RTL;
static int remove_fsdb(const char *pathname, const char *filename) CSEC_RTL;
static int rename_file(const char *, const char *, const char *, const char *) CSEC_RTL;
static int rename_memo(const char *, const char *, const char *, const char *) CSEC_RTL;
static int rename_fsdb(const char *, const char *, const char *, const char *) CSEC_RTL;
static int init_fileio(void) CSEC_RTL;
static void done_fileio(void) CSEC_RTL;
static int memory_prepare(void) CSEC_RTL;
static void memory_register(MemHandle hdl, MemPtr ptr) CSEC_RTL;
static void memory_unregister(MemHandle hdl, MemPtr ptr) CSEC_RTL;
static void memory_update(MemHandle hdl, MemPtr ptr) CSEC_RTL;
static void memory_release(void) CSEC_RTL;
static MemHandle memory_lookuphandle(MemPtr ptr) CSEC_RTL;
static int preformat_int_number_internal(char *buff, int blen, long long val, int base, int flags) CSEC_RTL;
static int preformat_int_number(struct format_desc *item) CSEC_RTL;
static int preformat_float_number(struct format_desc *item) CSEC_RTL;

/* to catch interesting spots in debug sessions */
static void dbg(void) {
	/* EMPTY */;
}

/*
 * might need to reference the PalmOS app creator
 */
static UInt32 CREATOR_CODE;

/*
 * errno.h
 */
/* { */

/*
 * can this be removed? used for FILE* stuff only (I hope),
 * and some Lua libraries (lauxlib, oslib, iolib) reference it
 */
static int errno_var;
int errno_(void) {
	return(errno_var);
}

/* } */

/*
 * stdlib.h
 */
/* { */
void exit(int status) {
	union app_event_desc ev;

	/* inject sys app exit event */
	memset(&ev, 0, sizeof(ev));
	ev.exit.retcode = status;
	app_add_event(APP_EVENT_EXIT, NULL);

	/* run event handler loop */
	app_run_event_handler(1);

	/* UNREACH */
}

/* RNG from http://www.c-faq.com/lib/rand.html (C-FAQ, 13.15) */
/* XXX use the Mersenne twister? */

#define a 48217
#define m 2147483647L
#define q (m / a)
#define r (m % a)

static Int32 rand_seed = 1;

/* the PMrand() routine */
static UInt32 PMrand(void) {
	Int32 hi, lo, test;

	hi = rand_seed / q;
	lo = rand_seed % q;
	test = a * lo - r * hi;
	if (test > 0) {
		rand_seed = test;
	} else {
		rand_seed = test + m;
	}
	return(rand_seed);
}
#undef a
#undef m
#undef q
#undef r

int rand(void) {
	Int32 rc;

	/*
	 * get the next random number,
	 * transform it into the RAND_MAX range
	 * (% RANDMAX is considered bad practice,
	 * use the high order bits)
	 */
	rc = PMrand();
	rc >>= 16;

	return(rc);
}

void srand(unsigned int seed) {
	rand_seed = seed;
}

int system(const char *command) {
	/* unsupported */
	/* XXX TODO via APP_EVENT_* and PalmOS launch? */
	return(-2); /* ENOENT undeclared */
}

char *getenv(const char *name) {
	/*
	 * XXX TODO implement with a file or memo document?
	 * it's used for loadlib paths, LUA_INIT, in examples
	 */
	return(NULL);
}

/*
 * we could use MemPtrRecoverHandle() but still need a list of all allocated
 * memory since we need to release them upon shutdown
 *
 * we could use the MemPtrSize(), MemPtrResize(), MemPtrUnlock() shortcuts
 */

static int memory_prepare(void);
static void memory_register(MemHandle hdl, MemPtr ptr);
static void memory_unregister(MemHandle hdl, MemPtr ptr);
static void memory_update(MemHandle hdl, MemPtr ptr);
static void memory_release(void);
static MemHandle memory_lookuphandle(MemPtr ptr);

void *malloc(const size_t size) {
	MemHandle hdl;
	MemPtr ptr;

	hdl = MemHandleNew(size);
	if (! hdl)
		return(NULL);
	ptr = MemHandleLock(hdl);
	memory_register(hdl, ptr);

	return(ptr);
}

void free(void *p) {
	MemHandle hdl;

	if (p == NULL)
		return;

	hdl = memory_lookuphandle(p);
	if (! hdl) {
		/* XXX not allocated by us (or lost track) -- what to do? */
		return;
	}
	memory_unregister(hdl, p);
	MemHandleUnlock(hdl);
	MemHandleFree(hdl);
}

void *realloc(void *ptr, size_t newsize) {
	MemHandle hdl;
	Err err;
	MemHandle hdl2;
	MemPtr ptr2;
	Int32 copylen;

	/* newsize -> 0, i.e. free the memory */
	if (newsize == 0) {
		free(ptr);
		return(NULL);
	}

	/* old ptr is NULL, i.e. first allocation */
	if (ptr == NULL) {
		ptr = malloc(newsize);
		return(ptr);
	}

	/* non zero newsize and previous pointer -> realloc */
	hdl = memory_lookuphandle(ptr);
	if (! hdl) {
		/* XXX not allocated by us (or lost track) -- what to do? */
		return(NULL);
	}

	/* "realloc" to the very same size? */
	if (newsize == MemHandleSize(hdl))
		return(ptr);

	/* try a resize first */
	MemHandleUnlock(hdl);
	err = MemHandleResize(hdl, newsize);
	ptr = MemHandleLock(hdl);
	if (! err) {
		memory_update(hdl, ptr);
		return(ptr);
	}
	/* resize failed, need to do the expensive stuff */

	/* get another memory chunk */
	ptr2 = malloc(newsize);
	if (ptr2 == NULL) {
		/* could not allocate new space either */
		return(NULL);
	}
	hdl2 = memory_lookuphandle(ptr2);

	/* copy over the old data */
	copylen = MemHandleSize(hdl);
	if (copylen > newsize)
		copylen = newsize;
	MemMove(ptr2, ptr, copylen);

	/* release the old chunk, return the new chunk */
	free(ptr);
	return(ptr2);
}

struct memory_item {
	MemHandle hdl;
	MemPtr ptr;
};

struct memory_table {
	Int32 size;
	Int32 used;
	MemHandle hdl;
	struct memory_item *items;
};

struct memory_table memory_list;

static int memory_prepare(void) {
	memset(&memory_list, 0, sizeof(memory_list));
	memory_list.size = 8;
	memory_list.used = 0;
	memory_list.hdl = MemHandleNew(memory_list.size * sizeof(memory_list.items[0]));
	if (! memory_list.hdl)
		return(-1);
	memory_list.items = MemHandleLock(memory_list.hdl);
	return(0);
}

static void memory_register(MemHandle hdl, MemPtr ptr) {
	Int32 idx;
	Int32 bytes;
	Err err;

	/* advance to the next slot, resize the table if needed */
	idx = memory_list.used;
	memory_list.used++;
	if (memory_list.used >= memory_list.size) {
		memory_list.size *= 2;
		bytes = memory_list.size * sizeof(memory_list.items[0]);
		MemHandleUnlock(memory_list.hdl);
		err = MemHandleResize(memory_list.hdl, bytes);
		if (err) {
			/* XXX memory exhausted, we harshly abort */
			exit(-1);
		}
		memory_list.items = MemHandleLock(memory_list.hdl);
	}

	/* store the new pair in the table */
	memory_list.items[idx].hdl = hdl;
	memory_list.items[idx].ptr = ptr;
}

static void memory_unregister(MemHandle hdl, MemPtr ptr) {
	Int32 idx;

	/* lookup the pair in the table */
	for (idx = 0; idx < memory_list.used; idx++) {
		if (memory_list.items[idx].hdl != hdl)
			continue;
		if (memory_list.items[idx].ptr != ptr)
			continue;

		/*
		 * remove the entry from the table
		 * (only remove, don't shrink table size)
		 */
		memory_list.used--;
		if (idx != memory_list.used) {
			memory_list.items[idx] = memory_list.items[memory_list.used];
		}
		break;
	}

	return;
}

static void memory_update(MemHandle hdl, MemPtr ptr) {
	Int32 idx;

	/* lookup the handle in the table */
	for (idx = 0; idx < memory_list.used; idx++) {
		if (memory_list.items[idx].hdl != hdl)
			continue;

		memory_list.items[idx].ptr = ptr;
		break;
	}

	return;
}

static void memory_release(void) {

	/* XXX TODO this is dirty, signal that condition in a different way? */
	if (memory_list.used > 0) {
		FrmCustomAlert(2300 /* ID_FORM_CUSTOM */, "unbalanced memory alloc", "", "");
	}

	while (memory_list.used > 0) {
		memory_list.used--;
		MemHandleUnlock(memory_list.items[memory_list.used].hdl);
		MemHandleFree(memory_list.items[memory_list.used].hdl);
	}
	MemHandleUnlock(memory_list.hdl);
	MemHandleFree(memory_list.hdl);

	memory_list.size = 0;
	memory_list.hdl = 0;
	memory_list.items = NULL;

	return;
}

static MemHandle memory_lookuphandle(MemPtr ptr) {
	Int32 idx;

	for (idx = 0; idx < memory_list.used; idx++) {
		if (memory_list.items[idx].ptr == ptr)
			return(memory_list.items[idx].hdl);
	}
	return(0);
}

unsigned long strtoul(const char *s, char **end, int base) {
	/* XXX TODO not correct yet, FIXME */
	return(strtol(s, end, base));
}

long strtol(const char *s, char **end, int base) {
	static char *digits = "0123456789abcdefghijklmnopqrstuvwxyz";

	long res;	/* result accu */
	int m;		/* minus */
	const char *p;	/* parse pointer */
	int b;		/* base */
	int n;		/* number of digits */
	int d;		/* current digit */
	int ec;		/* strchr() seems to clobber errno */

	/* reject invalid input parameters */
	if (s == NULL) {
		errno_var = EINVAL;
		/* say "nothing converted" together with EINVAL */
		if (end != NULL)
			*end = (char *)s; /* UNCONST */
		return(0);
	}
	if ((base != 0) && ((base < 2) || (base > strlen(digits)))) {
		errno_var = EINVAL;
		/* say "nothing converted" together with EINVAL */
		if (end != NULL)
			*end = (char *)s; /* UNCONST */
		return(0);
	}

	/* initialize internal status */
	res = 0;
	m = +1;
	p = s;
	b = base;
	n = 0;
	ec = 0;

	/* skip leading optional whitespace */
	while (isspace((int)*p)) {
		p++;
	}

	/* evaluate optional sign */
	switch (*p) {
	case '+':
		p++;
		break;
	case '-':
		m = -1;
		p++;
		break;
	default:
		/* EMPTY */
		break;
	}

	/* check optional prefix, maybe adjust base */
	if (*p == '0') {
		/* '0' may start a prefix or may be part of the number */
		p++;
		n++;
		switch (b) {
		case 16:
			/* hex forced? skip over prefix (but only if digits are present) */
			if (((*p == 'x') || (*p == 'X')) && (isxdigit((int)*(p + 1)))) {
				p++;
				n = 0;
			}
			break;
		case 0:
			/* auto detect oct or hex */
			b = 8;
			if (((*p == 'x') || (*p == 'X')) && (isxdigit((int)*(p + 1)))) {
				p++;
				n = 0;
				b = 16;
			}
			break;
		case 8:
		case 10:
		default:
			/* any other base? '0' is a valid digit, do nothing */
			/* EMPTY */
			break;
		}
	} else if (b == 0) {
		/* no prefix and no base specified? assume dec */
		b = 10;
	}

	/* process input (as long as possible) */
	while (*p != '\0') {
		char *s;
		long prev;

		d = *p;
		d = tolower(d);
		/* valid digit? */
		s = strchr(digits, d);
		if (s == NULL)
			break;
		d = s - digits;
		if (d >= b)
			break;
		p++;
		n++;
		/* merely consume remaining digits for this base after overflows */
		if (ec == ERANGE)
			continue;
		/* otherwise accumulate and check for overflow */
		prev = res;
		res *= b;
		if (((m > 0) && (res < 0)) || ((m < 0) && (res > 0)) || (res / b != prev)) {
			if (m > 0)
				res = LONG_MAX;
			else
				res = LONG_MIN;
			ec = ERANGE;
			continue;
		}
		if (m > 0) {
			prev = res;
			res += d;
			if ((res < 0) || (res - d != prev)) {
				res = LONG_MAX;
				ec = ERANGE;
			}
		} else {
			prev = res;
			res -= d;
			if ((res > 0) || (res + d != prev)) {
				res = LONG_MIN;
				ec = ERANGE;
			}
		}
	}
	errno_var = ec;

	/* not a single valid digit? act like we had read nothing at all */
	if (n == 0)
		p = s;

	/* return pointer to unconverted part (if requested) */
	if (end != NULL)
		*end = (char *)p; /* UNCONST */

	/* return conversion result */
	return(res);
}

double strtod(const char *nptr, char **endptr) {
	/* XXX TODO improve this naive implementation */
	const char *p;
	double rc;
	int sign;
	double fracweight;
	int expo;
	int digits;

	/* preset values */
	p = nptr;
	rc = 0.;

	/* skip leading whitespace */
	while (isspace(*p))
		p++;

	/* handle an optional sign */
	sign = +1;
	if (*p == '+') {
		sign = +1;
		p++;
	} else if (*p == '-') {
		sign = -1;
		p++;
	}

	/* handle the (actually optional) integer part */
	digits = 0;
	while ((*p >= '0') && (*p <= '9')) {
		digits++;
		rc *= 10.;
		rc += *p - '0';
		p++;
	}

	/* handle optional fractional part */
	if (*p == '.') {
		p++;
		fracweight = 1.;
		while ((*p >= '0') && (*p <= '9')) {
			digits++;
			fracweight /= 10.;
			rc += (*p - '0') * fracweight;
			p++;
		}
	}

	/* must have seen digits until here (before or after the decimal) */
	if (! digits) {
		/* format error, "rewind" and bail out */
		if (endptr != NULL)
			*endptr = (char *)nptr;
		return(0.);
	}
	/* we've consumed the input (at least) up to this position */
	nptr = p;

	/* apply the mantissa's sign */
	rc *= sign;

	/* handle optional exponent parts */
	if ((*p == 'e') || (*p == 'E')) {
		p++;
		expo = 0;
		/* get the optional sign */
		sign = +1;
		if (*p == '+') {
			sign = +1;
			p++;
		} else if (*p == '-') {
			sign = -1;
			p++;
		}
		/* get the digits */
		digits = 0;
		while ((*p >= '0') && (*p <= '9')) {
			expo *= 10;
			expo += (*p - '0');
			digits++;
			p++;
		}
		/* no digits seen? completely ignore the 'eXXX' part */
		if (! digits) {
			if (endptr != NULL)
				*endptr = (char *)nptr;
			return(rc);
		}
		/* "shift" the mantissa according to the exponent */
		fracweight = (sign > 0) ? 10. : .1;
		while (expo != 0) {
			rc *= fracweight;
			expo--;
		}
		/* we've successfully consumed more input */
		nptr = p;
	}

	/* return the conversion result */
	if (endptr != NULL)
		*endptr = (char *)nptr;
	return(rc);
}

/* } */


/*
 * locale.h
 */
/* { */
struct lconv *localeconv(void) {
	/* XXX callers hopefully check for NULL (Lua does) */
	/* used in llex for the decimal point */
	return(NULL);
}

char *setlocale(int category, const char *locale) {
	/* TODO does PalmOS have the notion of "locale"? */
	/* used in os.setlocale(), i.e. maybe used in Lua apps */
	return(NULL);
}
/* } */


/*
 * string.h
 */
/* { */
int strcoll(const char *s1, const char *s2) {
	return(strcmp(s1, s2));
}

int strcmp(const char *s1, const char *s2) {
	return(StrCompare(s1, s2));
}

char *strerror(int errnum) {
	static char buffer[32];

	/* XXX TODO update these with more known errno codes */
	switch (errnum) {
	case EINVAL: snprintf(buffer, sizeof(buffer), "%s error", "EINVAL"); break;
	case ERANGE: snprintf(buffer, sizeof(buffer), "%s error", "ERANGE"); break;
	default: snprintf(buffer, sizeof(buffer), "error number %d", errnum); break;
	}

	return(buffer);
}

size_t strlen(const char *s) {
	return(StrLen(s));
}

char *strchr(const char *s, int c) {
	return(StrChr(s, c));
}
/* } */


/*
 * stdio.h
 */
/* used in lauxlib.c, lbaselib.c, ldblib.c, liolib.c */

/* { */

static FILE *stdin_var  = NULL;
static FILE *stdout_var = NULL;
static FILE *stderr_var = NULL;

/*
 * XXX TODO
 *
 * make stdio another "store" variant to eliminate the two step hierarchy in
 * all the logic?
 *
 * make support for "file storage name spaces" optional
 * - determine available cards with MemNumCards() and MemCardInfo()
 * - add another "par" storage variant?
 */

enum file_case {	/* tell "files" from special "streams" */
	FTC_STDIN,
	FTC_STDOUT,
	FTC_STDERR,
	FTC_REGFILE,
};

enum file_mode {	/* flags for file modes */
	FM_READ 	= (1 << 0),
	FM_WRITE 	= (1 << 1),
	FM_APPEND 	= (1 << 2),
	FM_BINARY 	= (1 << 3),
	FM_TEMP 	= (1 << 4),
};

enum file_store {	/* where the file's content is stored */
	FS_NONE,
	FS_FILE,	/* FileMgr, no backup/hotsync */
	FS_MEMO,	/* MemoDB, small and Editor available */
	FS_FSDB,	/* proprietary DB container, needs another lib */
};

struct file_type {	/* all data describing a file */
	enum file_case ftc;	/* regular or special? */
	int mode;	/* r/w, append, binary */
	long size;	/* absolute size */
	long pos;	/* current position */
	int eof;	/* at EOF */
	int err;	/* failed? for ferror() */
	/* handle to file content (file/memo/fsdb), tag and details */
	enum file_store storeid;
	union {
		struct {
			UInt16 cardno;
			FileHand handle;
		} file;
		struct {
			DmOpenRef dbp;
			UInt16 idx;
			MemHandle rec;
			Char *str;
			UInt16 hdrlen;
		} memo;
		struct {
			DmOpenRef dbp;
			UInt16 idx;
			/* XXX TODO */
		} fsdb;
	} store;
	struct {
		int avail;
		int chr;
	} unget;
};

/*
 * fopen() implementations for the storage types; receive the filename (path)
 * and a FILE * struct with ftc, mode and storeid; should setup store, size,
 * pos, eof and err upon success or non zero err upon failure
 */

static void fopen_file(const char *path, struct file_type *f) {
	UInt32 type, creator, mode;
	Err err;
	Int32 rc;

	type = 'FILE'; creator = CREATOR_CODE;
	mode = 0;
	mode |= (f->mode & FM_WRITE) ? fileModeReadWrite : fileModeReadOnly;
	mode |= (f->mode & FM_APPEND) ? fileModeAppend : 0;
	f->store.file.cardno = 0;	/* determine from path? */
	f->store.file.handle = FileOpen(f->store.file.cardno, path, type, creator, mode, &err);
	if (err != 0) {
		f->err = 1;
		return;
	}

	f->pos = FileTell(f->store.file.handle, &rc, &err);
	if (err != 0) {
		f->err = 1;
		return;
	}
	f->size = rc;
	f->eof = (FileEOF(f->store.file.handle)) ? 1 : 0;
	f->err = (FileError(f->store.file.handle)) ? 1 : 0;

	return;
}

/*
Int32 FileRead(stream, bufP, objSize, numObj, errP)
Int32 FileWrite(FileHand stream, const void *dataP, Int32 objSize, Int32 numObj, Err *errP)
Err FileSeek(FileHand stream, Int32 offset, FileOriginEnum origin)
Int32 FileTell(FileHand stream, Int32 *fileSizeP, Err *errP)
Err FileEOF(__stream__)
Err FileError(__stream__)
Err FileFlush(__stream__)
Err FileClearerr(__stream__)
 */


static void fopen_memo(const char *path, struct file_type *f) {
	/* XXX TODO */
	f->err = 1;
	return;

#if 0
	Err rc;
	DmOpenRef dbp;
	MemHandle rec;
	Char *s;
	UInt16 idx, maxidx;

	/*
	 * XXX TODO
	 * - use "categories" in the memo database?
	 * - prepend filenames with "Lua:" in the memo title?
	 * - add more "namespaces" like card/vfs/lfsdb/etc?
	 */

	/* lookup the file name in the file system (memo DB) */
	dbp = DmOpenDatabaseByTypeCreator('DATA', 'memo', dmModeReadWrite);
	if (dbp == NULL) {
		free(f);
		return(NULL);
	}
	maxidx = DmNumRecords(dbp);
	for (idx = 0; idx < maxidx; idx++) {
		rec = DmQueryRecord(dbp, idx);
		if (rec == NULL) {
			f->err = 1;
			break;
		}
		s = MemHandleLock(rec);
		if ((StrNCompare(s, path, pathlen) == 0) && (s[pathlen] == '\n')) {
			f->storeid = FS_MEMO;
			f->store.memo.dbp = dbp;
			f->store.memo.idx = idx;
			f->store.memo.rec = rec;
			f->store.memo.str = s;
			break;
		}
		MemHandleUnlock(rec);
	}
	if (f->err) {
		rc = DmCloseDatabase(dbp);
		free(f);
		return(NULL);
	}

	/* file not found but "write/append" enabled? */
	if ((f->storeid == FS_NONE) && ((f->mode & FM_WRITE) != 0)) {
		/* create a new record, append at the end */
		idx = dmMaxRecordIndex;
		rec = DmNewRecord(dbp, &idx, pathlen + 1);
		if (rec == NULL) {
			rc = DmCloseDatabase(dbp);
			free(f);
			return(NULL);
		}
		/* "balance" the DmNewRecord() call? */
		/* write the filename and an empty data part */
		rec = DmGetRecord(dbp, idx);
		if (rec == NULL) {
			rc = DmCloseDatabase(dbp);
			free(f);
			return(NULL);
		}
		s = MemHandleLock(rec);
		/* DmStrCopy(rec, 0, path); */
		DmWrite(rec, 0, path, pathlen);
		DmWrite(rec, pathlen, "\n", 1);
		MemHandleUnlock(rec);
		DmReleaseRecord(dbp, idx, true);

		rec = DmQueryRecord(dbp, idx);
		if (rec == NULL) {
			rc = DmCloseDatabase(dbp);
			free(f);
			return(NULL);
		}
		s = MemHandleLock(rec);
		f->storeid = FS_MEMO;
		f->store.memo.dbp = dbp;
		f->store.memo.idx = idx;
		f->store.memo.rec = rec;
		f->store.memo.str = s;
	}
	/*
	 * now the "file" is open in read only mode, the filename is
	 * in the first line, the (optional) file content follows then
	 */

	/*
	 * there is no API to fetch the record sice (none I can see),
	 * so we (have to) only accept ASCIIZ strings as file content
	 */
	f->size = StrLen(s);
	f->store.memo.hdrlen = StrLen(path) + 1;
	f->size -= f->store.memo.hdrlen;

	f->pos = (f->mode & FM_APPEND) ? f->size : 0;
	f->eof = (f->pos == f->size) ? 1 : 0;
	f->err = 0;

	/* release stuff */
	f->store.memo.str = NULL;
	MemHandleUnlock(f->store.memo.rec);
	f->store.memo.rec = NULL;
#endif
}

static void fopen_fsdb(const char *path, struct file_type *f) {
	/* XXX TODO */
	f->err = 1;
	return;
}

FILE *fopen(const char *path, const char *mode) {
	int pathlen;
	struct file_type *f;
	const char *md;

	/* check parameters */
	if ((path == NULL) || (*path == '\0'))
		return(NULL);
	if ((mode == NULL) || (*mode == '\0'))
		return(NULL);
	pathlen = StrLen(path);

	/* allocate a description */
	f = malloc(sizeof(*f));
	if (f == NULL)
		return(NULL);
	MemSet(f, sizeof(*f), 0);

	/* assume this is (will be) a regular file */
	f->ftc = FTC_REGFILE;

	/* determine "mode" flags (from "[rwa][b+][b+]") */
	md = mode;
	switch (*md) {
	case 'r': f->mode |= FM_READ; md++; break;
	case 'w': f->mode |= FM_WRITE; md++; break;
	case 'a': f->mode |= FM_WRITE | FM_APPEND; md++; break;
	default: f->err = 1; break;
	}
	switch (*md) {
	case '+': f->mode |= FM_WRITE | FM_APPEND; md++; break;
	case 'b': f->mode |= FM_BINARY; md++; break;
	default: f->err = 1; break;
	}
	switch (*md) {
	case '+': f->mode |= FM_WRITE | FM_APPEND; md++; break;
	case 'b': f->mode |= FM_BINARY; md++; break;
	default: f->err = 1; break;
	}
	if (*md != '\0')
		f->err = 1;
	if (f->err) {
		free(f);
		return(NULL);
	}

	/*
	 * check whether the caller requested where the content
	 * should be stored (by using a certain prefix)
	 */
#define FS_CHECK_PREFIX(name, type) \
	if (StrNCompare(path, "//" ## name ## "/", strlen("//" ## name ## "/")) == 0) { \
		path += strlen("//" ## name); \
		f->storeid = type; \
	}
	f->storeid = FS_FILE;	/* the default storage */
	FS_CHECK_PREFIX("file", FS_FILE);
	FS_CHECK_PREFIX("memo", FS_MEMO);
	FS_CHECK_PREFIX("fsdb", FS_FSDB);
#undef FS_CHECK_PREFIX

	switch (f->storeid) {
	case FS_NONE: f->err = 1; break;
	case FS_FILE: fopen_file(path, f); break;
	case FS_MEMO: fopen_memo(path, f); break;
	case FS_FSDB: fopen_fsdb(path, f); break;
	default: f->err = 1; break;
	}
	if (f->err) {
		free(f);
		return(NULL);
	}

	/* return the opened file */
	return(f);
}

/*
 * fclose() implementations for the storage types, receives a file,
 * returns 0 on success
 */

static int fclose_file(struct file_type *f) {
	int rc;

	rc = 0;
	if (FileFlush(f->store.file.handle) != 0) {
		rc = -1;
	}
	if (FileClose(f->store.file.handle) != 0) {
		rc = -1;
	}
	f->store.file.handle = 0;
	f->err = (rc == 0) ? 0 : 1;
	return(0);
}

static int fclose_memo(struct file_type *f) {
	if (f->store.memo.str != NULL) {
		f->store.memo.str = NULL;
		MemHandleUnlock(f->store.memo.rec);
	}
	/* no need(?) to release the record */
	if (f->store.memo.dbp != NULL) {
		DmCloseDatabase(f->store.memo.dbp);
		f->store.memo.dbp = NULL;
	}
	f->err = 0;
	return(0);
}

static int fclose_fsdb(struct file_type *f) {
	/* XXX TODO */
	f->err = 0;
	return(-1);
}

/*
 * XXX
 * factor out alloc() and free() to use them
 * in fopen(), fclose() and rtl init()/done()?
 * keep a list of open files to close them
 * upon shutdown
 */
int fclose(FILE *f) {
	int rc;

	if (f == NULL)
		return(-1);

	rc = 0;

	if (fflush(f) != 0) {
		rc = -1;
	}

	switch (f->ftc) {
	case FTC_STDIN:
	case FTC_STDOUT:
	case FTC_STDERR:
		/* nothing to do */
		rc = 0;
		break;
	case FTC_REGFILE:
		switch (f->storeid) {
		case FS_FILE: rc = fclose_file(f); break;
		case FS_MEMO: rc = fclose_memo(f); break;
		case FS_FSDB: rc = fclose_fsdb(f); break;
		default: f->err = 1; break;
		}
		break;
	}

	if (f->err) {
		rc = f->err;
	}
	free(f);
	return(rc);
}

struct format_desc {
	struct format_desc *next;	/* linked list */
	int argidx;		/* index in fmt string */
	struct {
		int present:1;
		int number;
	} position;
	struct {
		int present:1;
		int alternate:1;
		int padzero:1;
		int alignleft:1;
		int blanksign:1;
		int alwayssign:1;
		int septhousand:1;
	} flags;
	struct {
		int present:1;
		int fromarg:1;
		int negative:1;
		int number;
	} width;
	struct {
		int present:1;
		int fromarg:1;
		int negative:1;
		int number;
	} precision;
	enum fmt_modifier {
		FMT_MODI_NONE,
		FMT_MODI_SHORT_SHORT,
		FMT_MODI_SHORT,
		FMT_MODI_LONG,
		FMT_MODI_LONG_LONG,
		FMT_MODI_LONG_DOUBLE,
		FMT_MODI_QUAD,
		FMT_MODI_MAXINT,
		FMT_MODI_SIZET,
		FMT_MODI_PTRDIFFT,
	} modifier;
	enum fmt_conversion {
		FMT_CONV_NONE,
		FMT_CONV_SIGNED_INT,
		FMT_CONV_UNSIGNED_INT,
		FMT_CONV_OCTAL,
		FMT_CONV_HEX_LOWER,
		FMT_CONV_HEX_UPPER,
		FMT_CONV_EXP_LOWER,
		FMT_CONV_EXP_UPPER,
		FMT_CONV_FLOAT_LOWER,
		FMT_CONV_FLOAT_UPPER,
		FMT_CONV_G_E_F_LOWER,
		FMT_CONV_G_E_F_UPPER,
		FMT_CONV_FLHEX_LOWER,
		FMT_CONV_FLHEX_UPPER,
		FMT_CONV_CHAR,
		FMT_CONV_STRING,
		FMT_CONV_PTR,
		FMT_CONV_STORE,
	} conversion;
	const char *fmt_start_pos;
	const char *fmt_conv_pos;
	union {
		signed long long sll;
		unsigned long long ull;
		double dbl;
		char chr;
		char *str;
		void *ptr;
	} parameter;
	struct {
		int needed;
		char *buffer;
	} formatted;
};

/* XXX TODO implement alloc() and free() for the format_desc structure */

/*
 * print an integer number into a buffer,
 * used for int types as well as for the
 * integer/fractional/exponent parts of floats
 */
enum preformat_flags {
	/* PINF_LEFT  = (1 << 0), */ /* left adjusting is done outside, not here */
	PINF_ZERO  = (1 << 1),
	PINF_SIGN  = (1 << 2),
	PINF_BLANK = (1 << 3),
	PINF_CAPS  = (1 << 4),
};

static int preformat_int_number_internal(char *buff, int blen, long long val, int base, int flags) {
	static char digits_lower[] = "0123456789abcdefghijklmnopqrstuvwxyz";
	static char digits_upper[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

	int idx;
	int m;	/* minus */
	int d;	/* digit */
	char *digits;

	/* parameter check (would be a programming error) */
	if (buff == NULL)
		return(-1);
	if (blen < 0)
		return(-1);
	if ((base < 2) || (base > strlen(digits_lower)))
		return(-1);

	/* prepare the "alphabet" for the digits */
	digits = (flags & PINF_CAPS) ? digits_upper : digits_lower;

	/* store the sign for later */
	m = 1;
	if (val < 0) {
		m = -1;
		val = -val;
	}

	/* build the number "from behind" */
	idx = blen;
	buff[idx] = '\0'; idx--;
	do {
		d = val % base;
		val /= base;
		buff[idx] = digits[d]; idx--;
	} while (val > 0);

	/* fill up with zeros when requested */
	if (flags & PINF_ZERO) {
		int to;
		if ((m < 0) || (flags & (PINF_SIGN | PINF_BLANK)))
			to = 1;
		else
			to = 0;
		while (idx >= to) {
			buff[idx] = '0'; idx--;
		}
	}

	/* put the sign in front */
	if (m < 0) {
		buff[idx] = '-'; idx--;
	} else if (flags & PINF_SIGN) {
		buff[idx] = '+'; idx--;
	} else if (flags & PINF_BLANK) {
		buff[idx] = ' '; idx--;
	}

	/* move to the buffer's front if not already there */
	idx++; /* position on the last character written */
	if (idx > 0) {
		memmove(buff, buff + idx, strlen(buff + idx) + 1);
	}

	return(0);
}

/* determine needed space for the number and "in bypassing" get a preformatted copy */
static int preformat_int_number(struct format_desc *item) {
	int szneeded;
	long long ival;	/* XXX ignore for the moment that unsigned has a wider range */
	int base;
	long long ival2;
	int flags;

	if (item == NULL)
		return(-1);

	/* get the value to print, the base to use; start with zero length */
	switch (item->conversion) {
	case FMT_CONV_SIGNED_INT:
		ival = item->parameter.sll;
		break;
	case FMT_CONV_UNSIGNED_INT:
	case FMT_CONV_OCTAL:
	case FMT_CONV_HEX_LOWER:
	case FMT_CONV_HEX_UPPER:
		ival = item->parameter.ull;
		break;
	case FMT_CONV_PTR:
		ival = (long)item->parameter.ptr;
		break;
	default:
		return(-1);
		/* UNREACH */
		break;
	}
	base = 10;
	switch (item->conversion) {
	case FMT_CONV_OCTAL:
		base = 8;
		break;
	case FMT_CONV_HEX_LOWER:
	case FMT_CONV_HEX_UPPER:
	case FMT_CONV_PTR:
		base = 16;
		break;
	default:
		/* EMPTY */
		break;
	}
	szneeded = 0;

	/* make sure we won't output "negative" values for unsigned conversions */
	/* XXX is this correct? shall we mask out bits instead of unary minus? */
	switch (item->conversion) {
	case FMT_CONV_UNSIGNED_INT:
	case FMT_CONV_OCTAL:
	case FMT_CONV_HEX_LOWER:
	case FMT_CONV_HEX_UPPER:
	case FMT_CONV_PTR:
		if (ival < 0) {
			ival = -ival;
		}
		break;
	default:
		/* EMPTY */
		break;
	}

	/* prepare the second step of actual printing */
	ival2 = ival; /* need another copy after the first was mangled */
	flags = 0;

	/* add room for the sign */
	if (ival < 0) {
		szneeded++;
		flags |= PINF_SIGN;
	} else if (item->flags.present) {
		if (item->flags.blanksign) {
			szneeded++;
			flags |= PINF_BLANK;
		} else if (item->flags.alwayssign) {
			szneeded++;
			flags |= PINF_SIGN;
		}
		if (item->flags.padzero)
			flags |= PINF_ZERO;
	}

	/* determine needed room for digits */
	if (ival < 0) {
		ival = -ival;
	}
	do {	/* increment for zero values, too */
		szneeded++;
		ival /= base;
	} while (ival > 0);

	/* use at minimum width or precision chars */
	if (item->width.present) {
		if (szneeded < item->width.number)
			szneeded = item->width.number;
	}
	if (item->precision.present) {
		if (szneeded < item->precision.number)
			szneeded = item->precision.number;
	}

	/* allocate a temporary buffer */
	item->formatted.needed = szneeded;
	item->formatted.buffer = malloc(szneeded + 1);
	if (item->formatted.buffer == NULL) {
		return(-1);
	}
	memset(item->formatted.buffer, 0, szneeded + 1);

	/* preformat the value into the temp buffer */
	if (preformat_int_number_internal(item->formatted.buffer, szneeded, ival2, base, flags) != 0) {
		return(-1);
	}

	/* shift around when needed (right justify) */
	/* XXX TODO */

	return(szneeded);
}

#include "../contrib/pdouble/pdouble.h"

/* the floating point counterpart to preformat_int_number() */
static int preformat_float_number(struct format_desc *item) {
	static char digits_lower[] = "0123456789abcdef";
	static char digits_upper[] = "0123456789ABCDEF";

	double dval, dcopy;
	int base;
	int dig, prec;
	char *wptr;
	char *digits;
	double mantissa;
	UInt16 exppart;

	if (item == NULL)
		return(-1);

	/* get the number, and base */
	switch (item->conversion) {
	case FMT_CONV_EXP_LOWER:
	case FMT_CONV_EXP_UPPER:
	case FMT_CONV_FLOAT_LOWER:
	case FMT_CONV_FLOAT_UPPER:
	case FMT_CONV_G_E_F_LOWER:
	case FMT_CONV_G_E_F_UPPER:
		/* "regular" decimal floating point */
		dval = item->parameter.dbl;
		base = 10;
		break;
	case FMT_CONV_FLHEX_LOWER:
	case FMT_CONV_FLHEX_UPPER:
		/* hex representation of floating point */
		dval = item->parameter.dbl;
		base = 16;
		break;
	default:
		/* unsupported type */
		return(-1);
		/* UNREACH */
		break;
	}

	digits = digits_lower;
	switch (item->conversion) {
	case FMT_CONV_EXP_UPPER:
	case FMT_CONV_FLOAT_UPPER:
	case FMT_CONV_G_E_F_UPPER:
	case FMT_CONV_FLHEX_UPPER:
		digits = digits_upper;
		break;
	default:
		/* EMPTY */
		break;
	}
	/* default precision needs to suffice for all significant bits of a double for 'a/A' conversions */
	prec = (item->precision.present) ? item->precision.number : 15;

	/* allocate room for the conversion result */
	item->formatted.needed = 30;
	item->formatted.buffer = malloc(item->formatted.needed);
	if (item->formatted.buffer == NULL)
		return(-1);
	memset(item->formatted.buffer, 0, item->formatted.needed);

	/* start the result with the sign when needed */
	wptr = item->formatted.buffer;
	if (dval < 0) {
		*wptr = '-'; wptr++;
		dval = -dval;
	}

	/* handle the hex format, it's rather straight forward */
	if (base == 16) {

		/* write the "0x" prefix */
		*wptr = '0'; wptr++;
		*wptr = (digits == digits_upper) ? 'X' : 'x'; wptr++;

		/*
		 * write the mantissa (one digit before the point,
		 * as many after the point as precision specifies)
		 */
		mantissa = frexp(dval, &exppart);
		mantissa *= base;
		dig = (int)mantissa;
		*wptr = digits[dig]; wptr++;
		*wptr = '.'; wptr++;
		do {
			mantissa *= base;
			dig = (int)mantissa;
			*wptr = digits[dig]; wptr++;
			prec--;
		} while ((mantissa > 0) && (prec > 0));

		/*
		 * write the exponent part
		 * (ought that really be a decimal exponent?)
		 *
		 * do it here in place ourselves instead of
		 * calling the flexible routine with lots of
		 * options we don't make use of?
		 */
		*wptr = (digits == digits_upper) ? 'P' : 'p'; wptr++;
		preformat_int_number_internal(wptr, 8, exppart, 10, 0);

		return(strlen(item->formatted.buffer));
	}

	/* use the pdouble.c code for regular output (inf/nan/expo/sci/float) */
	printDouble(dval, item->formatted.buffer);
	return(strlen(item->formatted.buffer));

	/* handle 0 here already, it's simple :) */
	if (dval == 0.) {
		*wptr = '0'; wptr++;
		/* write "0" or "0."? I assume the former */
		return(strlen(item->formatted.buffer));
	}

	/*
	 * see whether we can represent the value with at most
	 * "prec - 2" digits (either before or after the point,
	 * at least one digit "on the other side" of the point)
	 */
	dcopy = dval;	/* get a copy of the value */
	dig = prec - 2;	/* assumed digits we test against */
	if (dcopy < 1) {
		/* values below 1, aka 0.xxx */
		while ((dig > 0) && (dcopy != 0)) {
			dcopy *= 10;
			dcopy -= (int)dcopy;
			dig--;
		}
	} else {
		/* values >= 1. */
		if (log10(dcopy) >= dig)
			dcopy = 1;
		else
			dcopy = 0;
	}
	if (dcopy == 0) {

		/* at most "prec - 2" digits before the point */
		preformat_int_number_internal(wptr, prec - 2, (long)dval, 10, 0);
		prec -= strlen(wptr);
		wptr += strlen(wptr);

		/* the decimal point */
		*wptr = '.'; wptr++;
		prec--;

		/* (at most) "precision" digits after the point */
		dval -= (int)dval;	/* XXX huh, how to reliably trim off the integer part? */
		while ((prec > 0) && (dval > 0)) {
			dval *= 10.;
			/* round up the last digits (disregard the input's sign) */
			if (prec == 1)
				dval += .5;
			dig = (int)dval;
			*wptr = digits[dig]; wptr++;
			dval -= (int)dval;
			prec--;
		}

		/* handle a funny effect: 4.3 becomes 4.29999999999a */
		/* XXX optional: move rounding the number before issuing digits */
		while (wptr > item->formatted.buffer) {
			if (*(wptr -1) - '0' < base) {
				break;
			}
			wptr--;
			*wptr = '\0';
			if (*(wptr - 1) == '.')
				wptr--;
			(*(wptr - 1))++;
		}

		/* trim off trailing zero digits */
		while ((wptr > item->formatted.buffer) && (*(wptr - 1) == '0')) {
			wptr--;
			*wptr = '\0';
		}

		/* trim off the dot if there are no fractional digits */
		while ((wptr > item->formatted.buffer) && (*(wptr - 1) == '.')) {
			wptr--;
			*wptr = '\0';
		}

		return(strlen(item->formatted.buffer));
	}
	/* we seem to need "scientific" presentation with exponents */

	/* normalize the mantissa to 1..9, determine the 10-base exponent */
	mantissa = dval;
	exppart = 0;
	while (mantissa < 1.) {
		mantissa *= 10.;
		exppart--;
	}
	while (mantissa > 10.) {
		mantissa /= 10.;
		exppart++;
	}

	/* the digit before the point */
	dig = (int)mantissa;
	*wptr = digits[dig]; wptr++;

	/* the decimal point */
	*wptr = '.'; wptr++;

	/* "precision" digits after the point */
	do {
		mantissa -= (int)mantissa;
		mantissa *= base;
		if (prec == 1)
			mantissa += .5;
		dig = (int)mantissa;
		*wptr = digits[dig]; wptr++;
		prec--;
	} while ((mantissa > 0) && (prec > 0));

	/* XXX TODO optionally trim trailing zero digits and dot here, too */

	/* the exponent (yes, optional in case we reuse this logic later) */
	if (exppart != 0) {
		/*
		 * write the exponent part
		 *
		 * do it here in place ourselves instead of
		 * calling the flexible routine with lots of
		 * options we don't make use of?
		 */
		*wptr = (digits == digits_upper) ? 'E' : 'e'; wptr++;
		preformat_int_number_internal(wptr, 8, exppart, 10, 0);
	}

	return(strlen(item->formatted.buffer));
}

/* XXX can StrPrintf() help us here? although it has no length check */
static int formatted_write(const char *fmt, va_list args, char **s, int *l) {
	int sz, szneeded;
	int argpos;
	const char *p;
	long long ival;
	double dval;
	char *sval;
	void *pval;
	int rc;
	char *w;

	struct format_desc *poslist = NULL;
	struct format_desc *item, **parent;

	/*
	 * handle fprintf() format specs:
	 * - starts with '%'
	 * - zero or more flags
	 *   - '#' for "alternate form" (not supported)
	 *   - '0' for zero padding
	 *   - '-' for left adjustment
	 *   - ' ' for "blank before positive number"
	 *   - '+' for "always place sign before numbers"
	 *   - '\'' (SUS) for "group thousands" (not supported)
	 * - optional minumum field width (digits, non zero first;
	 *   alternatively '*', consuming an int argument, negative
	 *   means "left adjustment"), never truncate
	 * - optional precision: starting with '.'; optionally followed by
	 *   digits, or by '*' consuming an int parameter; negative or missing
	 *   means zero; minimum number of digits for diouxX, maximum number of
	 *   digits after radix char for aAeEfF, maximum number of significant
	 *   digits for gG, maximum number of chars for sS
	 * - optional length modifier
	 *   - 'hh' for "diouxX for char" or "n for ptr to char"
	 *   - 'h' for "short int"
	 *   - 'l' for "long int" or "wint_t for c" or "wchar_t for s"
	 *   - 'll' for "long long int"
	 *   - 'L' for "long double for aAeEfFgG"
	 *   - 'q' for "quad" ('do not use', not portable)
	 *   - 'j' for "[u]intmax_t integer"
	 *   - 'z' for "size_t" ('do not use', not portable)
	 *   - 't' for "ptrdiff_t"
	 *   - only 'h', 'l' and 'L' are SUSv2
	 * - ends with a "conversion specifier"
	 *   - d, i for "int to signed decimal", precision is min number of digits
	 *   - o, u, x, X for "unsigned int to oct/dec/hex
	 *   - e, E for "double is rounded and converted to [-]d.ddde+-dd"
	 *   - f, F for "double is rounded and converted to [-]ddd.ddd"
	 *   - g, G for "f or e format depending on exponent"
	 *   - a, A for "double to [-]0xh.hhhhp+-d" (hex float; C99, not SUSv2)
	 *   - c for "int to char" without 'l', or "wint to mb seq" with 'l'
	 *   - s for "const char * as ASCIIZ" without 'l', or
	 *     "const wchar_t * as wchar sequence" with 'l'
	 *   - C is 'lc', don't use, not portable
	 *   - S is 'ls', don't use, not portable
	 *   - p for "void * as %#x"
	 *   - n for "store the number of chars currently written so far to int*"
	 *   - % will print a percent sign
	 * - no support for '%n$' style references
	 *
	 * => results in a format pattern
	 * '%' ([1-9][0-9]*\$)? ([#0 +-']*)? (([1-9][0-9]*)|('*'))? ('.' (('*')|([1-9][0-9]*)))? (hh|h|l|ll|L|j|z|t)? [diouxXeEfFgGaAcsCSpn%]
	 */

	if (fmt == NULL)
		return(-1);
	if ((s == NULL) || (l == NULL))
		return(-1);
	*s = NULL;

	/* parse the format string and evaluate the parameters */
	sz = 0;
	argpos = 0;
	for (p = fmt; *p != '\0'; p++) {

		/* verbatim copy unless '%' detected */
		if (*p != '%') {
			sz++;
			continue;
		}
		/* skip over '%%' percent sign escapes */
		if (*(p + 1) == '%') {
			sz++;
			p++;
			continue;
		}

		/* allocate another position item */
		item = malloc(sizeof(*item));
		if (item == NULL) {
			sz = -1;
			break;
		}
		MemSet(item, sizeof(*item), 0);

		/* keep a reference where the '%' spec starts */
		item->fmt_start_pos = p;
		p++;

		/*
		 * TODO support positional args?  insist in all specs
		 * having this syntax as soon as the first spec had it
		 *
		 * we would need to
		 * - check for "[1-9][0-9]*\$" here without modifying
		 *   our current read pointer if this is the very first
		 *   format spec we see
		 * - get the "[1-9][0-9]*\$" reference here for the first
		 *   and every following (i.e. all) format specs when the
		 *   first one had that format
		 * - read all values (with their type information?) into
		 *   an array which later gets referenced by the format's
		 *   index upon actual formatting
		 *
		 * I'm afraid that support for positional args actually is
		 * impossible without va_copy() and three loops:  first one has
		 * to determine the number and type of references, then one has
		 * to va_arg() the parameters according to their type (they need
		 * not all have the int or double promo type), and only after
		 * the randomly ordered references and all arguments are
		 * available conversion and output construction can take place
		 */
		if (poslist == NULL) do {
			/* first format spec, check for positional type */
			const char *s;

			s = p;
			while ((*s >= '0') && (*s <= '9'))
				s++;
			if (s == p)
				break;
			if (*s != '$')
				break;
			argpos = 1;
			/*
			 * XXX build a list of args here already?
			 * so that the values are available when
			 * the format spec is complete; but do all
			 * parameters have an uniform width so
			 * they can be "taken" without knowing
			 * their type here? see the above longer
			 * comment
			 *
			 * how do positional args combine with width
			 * or precision specs from values ('*') and
			 * "store" specs ('%n')?  is it possible
			 * (since required) to specify something like
			 * printf("%1$2$*d", 42, 23)?
			 */
		} while (0);
		/*
		 * XXX for the moment we don't support positional specs
		 * (PalmOS lacks the va_copy() feature which would help)
		 */
		if (argpos) {
			sz = -1;
			break;
		}

		/*
		 * get position reference (all format specs need that if
		 * one of them had); no explicit check for positional spec
		 * is done here when the first spec had none, this is
		 * "achieved" by the spec not being a valid (remainder of
		 * a) format spec and bailing out with an error below
		 */
		if (argpos) {
			item->position.present = 1;
			item->position.number = 0;

			while ((*p >= '0') && (*p <= '9')) {
				item->position.number *= 10;
				item->position.number += *p - '0';
				p++;
			}
			if (item->position.number == 0) {
				sz = -1;
				break;
			}
			if (*p != '$') {
				sz = -1;
				break;
			}
			p++;
		}

		/* get flags */
		do {
			int handled;

			handled = 1;
			switch (*p) {
			case '#':
				item->flags.alternate = 1;
				break;
			case '0':
				item->flags.padzero = 1;
				break;
			case ' ':
				item->flags.blanksign = 1;
				break;
			case '+':
				item->flags.alwayssign = 1;
				break;
			case '-':
				item->flags.alignleft = 1;
				break;
			case '\'':
				item->flags.septhousand = 1;
				break;
			default:
				handled = 0;
				break;
			}
			if (! handled)
				break;
			item->flags.present = 1;
			p++;
		} while (1);

		/* get width */
		if (*p == '*') {
			item->width.present = 1;
			item->width.fromarg = 1;
			item->width.number = va_arg(args, int);
			item->width.negative = (item->width.number < 0) ? 1 : 0;
		/* XXX negative width without '*' impossible because of flags? */
		} else if ((*p >= '1') && (*p <= '9')) {
			item->width.present = 1;
			item->width.fromarg = 0;
			item->width.negative = 0;
			item->width.number = 0;
			while ((*p >= '0') && (*p <= '9')) {
				item->width.number *= 10;
				item->width.number += *p - '0';
				p++;
			}
		}

		/* get precision */
		if (*p == '.') {
			item->precision.present = 1;
			item->precision.fromarg = 0;
			item->precision.negative = 0;
			item->precision.number = 0;
			p++;
			if (*p == '*') {
				item->precision.fromarg = 1;
				item->precision.number = va_arg(args, int);
				item->precision.negative = (item->precision.number < 0) ? 1 : 0;
			} else if ((*p == '-') || ((*p >= '1') && (*p <= '9'))) {
				if (*p == '-') {
					item->precision.negative = 1;
					p++;
				}
				if ((*p < '1') || (*p > '9')) {
					/* cause an error below */
					p += StrLen(p);
				}
				while ((*p >= '0') && (*p <= '9')) {
					item->precision.number *= 10;
					item->precision.number += *p - '0';
					p++;
				}
				if (item->precision.negative) {
					item->precision.negative = 0;
					item->precision.number = 0;
				}
			}
		}

		/* get modifier */
		item->modifier = FMT_MODI_NONE;
		switch (*p) {
		case 'h':
			item->modifier = FMT_MODI_SHORT;
			p++;
			if (*p == 'h') {
				item->modifier = FMT_MODI_SHORT_SHORT;
				p++;
			}
			break;
		case 'l':
			item->modifier = FMT_MODI_LONG;
			p++;
			if (*p == 'l') {
				item->modifier = FMT_MODI_LONG_LONG;
				p++;
			}
			break;
		case 'L':
			item->modifier = FMT_MODI_LONG_DOUBLE;
			p++;
			break;
		case 'q':
			item->modifier = FMT_MODI_QUAD;
			p++;
			break;
		case 'j':
			item->modifier = FMT_MODI_MAXINT;
			p++;
			break;
		case 'z':
			item->modifier = FMT_MODI_SIZET;
			p++;
			break;
		case 't':
			item->modifier = FMT_MODI_PTRDIFFT;
			p++;
			break;
		default:
			/* EMPTY */
			break;
		}

		/*
		 * get conversion type, check modifier combination,
		 * consume argument, determine needed width
		 *
		 * XXX should va_arg() use the "default promotion size"
		 * for the appropriate types (always int and double
		 * unless "something long")?  or does va_arg() handle
		 * this already?
		 *
		 * XXX getting the value here would need to be deferred
		 * when we support positional specs, having va_copy()
		 * would be a great help or even essential
		 */
		item->fmt_conv_pos = p;
		item->conversion = FMT_CONV_NONE;
		szneeded = 0;
		ival = 0;
		dval = 0.;
		sval = pval = NULL;
		switch (*p) {
		case 'd':
		case 'i':
			item->conversion = FMT_CONV_SIGNED_INT;
			switch (item->modifier) {
			case FMT_MODI_NONE:
				ival = item->parameter.sll = va_arg(args, int);
				break;
			case FMT_MODI_SHORT_SHORT:
				ival = item->parameter.sll = va_arg(args, char);
				break;
			case FMT_MODI_SHORT:
				ival = item->parameter.sll = va_arg(args, short);
				break;
			case FMT_MODI_LONG:
				ival = item->parameter.sll = va_arg(args, long);
				break;
			case FMT_MODI_LONG_LONG:
				ival = item->parameter.sll = va_arg(args, long long);
				break;
			case FMT_MODI_QUAD:
				ival = item->parameter.sll = va_arg(args, Int32);
				break;
			case FMT_MODI_MAXINT:
				ival = item->parameter.sll = va_arg(args, Int32);
				break;
			case FMT_MODI_SIZET:
				ival = item->parameter.sll = va_arg(args, size_t);
				break;
			case FMT_MODI_PTRDIFFT:
				ival = item->parameter.sll = va_arg(args, ptrdiff_t);
				break;
			default:
				szneeded = -1;
				break;
			}
			break;
		case 'u':
			item->conversion = FMT_CONV_UNSIGNED_INT;
			switch (item->modifier) {
			case FMT_MODI_NONE:
				ival = item->parameter.ull = va_arg(args, unsigned int);
				break;
			case FMT_MODI_SHORT_SHORT:
				ival = item->parameter.ull = va_arg(args, unsigned char);
				break;
			case FMT_MODI_SHORT:
				ival = item->parameter.ull = va_arg(args, unsigned short);
				break;
			case FMT_MODI_LONG:
				ival = item->parameter.ull = va_arg(args, unsigned long);
				break;
			case FMT_MODI_LONG_LONG:
				ival = item->parameter.ull = va_arg(args, unsigned long long);
				break;
			case FMT_MODI_QUAD:
				ival = item->parameter.ull = va_arg(args, UInt32);
				break;
			case FMT_MODI_MAXINT:
				ival = item->parameter.ull = va_arg(args, UInt32);
				break;
			case FMT_MODI_SIZET:
				ival = item->parameter.ull = va_arg(args, size_t);
				break;
			case FMT_MODI_PTRDIFFT:
				ival = item->parameter.ull = va_arg(args, ptrdiff_t);
				break;
			default:
				szneeded = -1;
				break;
			}
			break;
		case 'o':
			item->conversion = FMT_CONV_OCTAL;
			switch (item->modifier) {
			case FMT_MODI_NONE:
				ival = item->parameter.ull = va_arg(args, int);
				break;
			case FMT_MODI_SHORT_SHORT:
				ival = item->parameter.ull = va_arg(args, char);
				break;
			case FMT_MODI_SHORT:
				ival = item->parameter.ull = va_arg(args, short);
				break;
			case FMT_MODI_LONG:
				ival = item->parameter.ull = va_arg(args, long);
				break;
			case FMT_MODI_LONG_LONG:
				ival = item->parameter.ull = va_arg(args, long long);
				break;
			default:
				szneeded = -1;
				break;
			}
			break;
		case 'x':
			item->conversion = FMT_CONV_HEX_LOWER;
			switch (item->modifier) {
			case FMT_MODI_NONE:
				ival = item->parameter.ull = va_arg(args, int);
				break;
			case FMT_MODI_SHORT_SHORT:
				ival = item->parameter.ull = va_arg(args, char);
				break;
			case FMT_MODI_SHORT:
				ival = item->parameter.ull = va_arg(args, short);
				break;
			case FMT_MODI_LONG:
				ival = item->parameter.ull = va_arg(args, long);
				break;
			case FMT_MODI_LONG_LONG:
				ival = item->parameter.ull = va_arg(args, long long);
				break;
			default:
				szneeded = -1;
				break;
			}
			break;
		case 'X':
			item->conversion = FMT_CONV_HEX_UPPER;
			switch (item->modifier) {
			case FMT_MODI_NONE:
				ival = item->parameter.ull = va_arg(args, int);
				break;
			case FMT_MODI_SHORT_SHORT:
				ival = item->parameter.ull = va_arg(args, char);
				break;
			case FMT_MODI_SHORT:
				ival = item->parameter.ull = va_arg(args, short);
				break;
			case FMT_MODI_LONG:
				ival = item->parameter.ull = va_arg(args, long);
				break;
			case FMT_MODI_LONG_LONG:
				ival = item->parameter.ull = va_arg(args, long long);
				break;
			default:
				szneeded = -1;
				break;
			}
			break;
		case 'e':
			item->conversion = FMT_CONV_EXP_LOWER;
			switch (item->modifier) {
			case FMT_MODI_NONE:
				dval = item->parameter.dbl = va_arg(args, double);
				break;
			case FMT_MODI_LONG_DOUBLE:
				dval = item->parameter.dbl = va_arg(args, long double);
				break;
			default:
				szneeded = -1;
				break;
			}
			break;
		case 'E':
			item->conversion = FMT_CONV_EXP_UPPER;
			switch (item->modifier) {
			case FMT_MODI_NONE:
				dval = item->parameter.dbl = va_arg(args, double);
				break;
			case FMT_MODI_LONG_DOUBLE:
				dval = item->parameter.dbl = va_arg(args, long double);
				break;
			default:
				szneeded = -1;
				break;
			}
			break;
		case 'f':
			item->conversion = FMT_CONV_FLOAT_LOWER;
			switch (item->modifier) {
			case FMT_MODI_NONE:
				dval = item->parameter.dbl = va_arg(args, double);
				break;
			case FMT_MODI_LONG_DOUBLE:
				dval = item->parameter.dbl = va_arg(args, long double);
				break;
			default:
				szneeded = -1;
				break;
			}
			break;
		case 'F':
			item->conversion = FMT_CONV_FLOAT_UPPER;
			switch (item->modifier) {
			case FMT_MODI_NONE:
				dval = item->parameter.dbl = va_arg(args, double);
				break;
			case FMT_MODI_LONG_DOUBLE:
				dval = item->parameter.dbl = va_arg(args, long double);
				break;
			default:
				szneeded = -1;
				break;
			}
			break;
		case 'g':
			item->conversion = FMT_CONV_G_E_F_LOWER;
			switch (item->modifier) {
			case FMT_MODI_NONE:
				dval = item->parameter.dbl = va_arg(args, double);
				break;
			case FMT_MODI_LONG_DOUBLE:
				dval = item->parameter.dbl = va_arg(args, long double);
				break;
			default:
				szneeded = -1;
				break;
			}
			break;
		case 'G':
			item->conversion = FMT_CONV_G_E_F_UPPER;
			switch (item->modifier) {
			case FMT_MODI_NONE:
				dval = item->parameter.dbl = va_arg(args, double);
				break;
			case FMT_MODI_LONG_DOUBLE:
				dval = item->parameter.dbl = va_arg(args, long double);
				break;
			default:
				szneeded = -1;
				break;
			}
			break;
		case 'a':
			item->conversion = FMT_CONV_FLHEX_LOWER;
			switch (item->modifier) {
			case FMT_MODI_NONE:
				dval = item->parameter.dbl = va_arg(args, double);
				break;
			case FMT_MODI_LONG_DOUBLE:
				dval = item->parameter.dbl = va_arg(args, long double);
				break;
			default:
				szneeded = -1;
				break;
			}
			break;
		case 'A':
			item->conversion = FMT_CONV_FLHEX_UPPER;
			switch (item->modifier) {
			case FMT_MODI_NONE:
				dval = item->parameter.dbl = va_arg(args, double);
				break;
			case FMT_MODI_LONG_DOUBLE:
				dval = item->parameter.dbl = va_arg(args, long double);
				break;
			default:
				szneeded = -1;
				break;
			}
			break;
		case 'c':
			item->conversion = FMT_CONV_CHAR;
			/* no support for wide or mb chars */
			ival = item->parameter.chr = va_arg(args, char);
			break;
		case 's':
			item->conversion = FMT_CONV_STRING;
			/* no support for wide char strings */
			sval = item->parameter.str = va_arg(args, char *);
			break;
		case 'p':
			item->conversion = FMT_CONV_PTR;
			pval = item->parameter.ptr = va_arg(args, void *);
			break;
		case 'n':
			item->conversion = FMT_CONV_STORE;
			switch (item->modifier) {
			case FMT_MODI_NONE:
				item->parameter.ptr = va_arg(args, int *);
				break;
			case FMT_MODI_SHORT_SHORT:
				item->parameter.ptr = va_arg(args, char *);
				break;
			case FMT_MODI_SHORT:
				item->parameter.ptr = va_arg(args, short *);
				break;
			case FMT_MODI_LONG:
				item->parameter.ptr = va_arg(args, long *);
				break;
			case FMT_MODI_LONG_LONG:
				item->parameter.ptr = va_arg(args, long long *);
				break;
			default:
				szneeded = -1;
				break;
			}
			break;
		default:
			szneeded = -1;
			break;
		}

		/*
		 * bail out on format errors as well as unsupported formats
		 *
		 * XXX we abort the loop and reject the whole request,
		 * to skip this format and to cite it in verbatim form
		 * instead of expanding it would get the application's
		 * idea of passing arguments and our interpreting them
		 * out of sync which may have rather dramatic consequences
		 * (think pointers)
		 */
		if (item->conversion == FMT_CONV_NONE) {
			szneeded = -1;
		}
		if (szneeded < 0) {
			free(item);
			sz = -1;
			break;
		}

		/* append the item to the list */
		item->argidx = 1;
		for (parent = &poslist; *parent != NULL; parent = &((*parent)->next))
			item->argidx++;
		*parent = item;
		/* make non positional args look like "strictly sequentially numbered" */
		if (! item->position.present)
			item->position.number = item->argidx;

/* XXX TODO further implementation */

		/*
		 * determine needed size from the argument,
		 * then adjust with the format constraints
		 *
		 * get needed chars from value,
		 * use at minimum width chars,
		 * use at most precision chars for floats' fractions,
		 *   or use at most precision chars for G floats,
		 *   or use at most precision chars for s strings,
		 * add optional sign/space room,
		 * consider numerical base and value size (log),
		 *
		 * XXX this needs to be done in another loop when we
		 * support positional specs, only then the values for
		 * the format specs are known/present (unless we build
		 * the list of arguments in the moment we detect that
		 * positional args are used -- if that's possible)
		 */

		szneeded = 0;

		/* treat pointers as integers to print the value */
		switch (item->conversion) {
		case FMT_CONV_PTR:
			ival = (long)pval;
			break;
		default:
			/* EMPTY */
			break;
		}

		/* determine needed size */
		switch (item->conversion) {
		case FMT_CONV_SIGNED_INT:
		case FMT_CONV_UNSIGNED_INT:
		case FMT_CONV_OCTAL:
		case FMT_CONV_HEX_LOWER:
		case FMT_CONV_HEX_UPPER:
		case FMT_CONV_PTR:
			/* integer types */
			/* already setup formatted text and use its length */
			szneeded = preformat_int_number(item);
			break;
		case FMT_CONV_CHAR:
			/* char type */
			szneeded = 1;
			/* use at minimum width chars */
			if (item->width.present) {
				if (szneeded < item->width.number)
					szneeded = item->width.number;
			}
			break;
		case FMT_CONV_STRING:
			/* string types */
			szneeded = strlen(sval);
			/* use at minimum width, at maximum precision chars */
			if (item->width.present) {
				if (szneeded < item->width.number)
					szneeded = item->width.number;
			}
			if (item->precision.present) {
				if (szneeded > item->precision.number)
					szneeded = item->precision.number;
			}
			break;
		case FMT_CONV_STORE:
			/* pseudo format, no output */
			szneeded = 0;
			break;
		case FMT_CONV_EXP_LOWER:
		case FMT_CONV_EXP_UPPER:
		case FMT_CONV_FLOAT_LOWER:
		case FMT_CONV_FLOAT_UPPER:
		case FMT_CONV_G_E_F_LOWER:
		case FMT_CONV_G_E_F_UPPER:
		case FMT_CONV_FLHEX_LOWER:
		case FMT_CONV_FLHEX_UPPER:
			/* XXX TODO all floats are still to be done */
			szneeded = -1;
			/* already setup formatted text and use its length */
			szneeded = preformat_float_number(item);
			break;
		default:
			/* programming error, unhandled case */
			szneeded = -1;
			break;
		}

		if (szneeded < 0) {
			/* XXX what to do on such implementation error? */
			sz = -1;
			break;
		}

		sz += szneeded;

	} /* end of format string and parameters list */

	/* prepare successful exit code, pessimize below upon failure */
	rc = 0;

	/*
	 * allocate the buffer plus room for a NUL terminator
	 * (the empty string is a valid printf() format string)
	 */
	if (sz >= 0) {
		*s = malloc(sz + 1);
		*l = 0;
	}
	if (sz < 0)
		rc = -1;
	if (*s == NULL)
		rc = -1;

	/*
	 * put together the formatted output
	 * (unless errors have been setup above
	 * or have happened in the loop)
	 */
	p = fmt;	/* format string, input */
	w = *s;		/* write pointer, output */
	item = poslist;	/* data to expand macros with */
	while (rc == 0) {
		const char *end;
		int use, fill;

		/*
		 * copy the constant part from the format string,
		 * handle the "%%" escapes
		 */
		if (item == NULL) {
			end = p + strlen(p);
		} else {
			end = item->fmt_start_pos;
		}
		while (p < end) {
			if (*p == '%') {
				p++;
				/* skip width when supported? */
				if (*p != '%') {
					rc = -1;
					break;
				}
			}
			*w = *p;
			w++;
			p++;
		}

		/* no more format specs? we're done then */
		if (item == NULL)
			break;

		/* we have a format spec and need to insert the data */
		switch (item->conversion) {
		/* XXX TODO */
		case FMT_CONV_SIGNED_INT:
		case FMT_CONV_UNSIGNED_INT:
		case FMT_CONV_OCTAL:
		case FMT_CONV_HEX_LOWER:
		case FMT_CONV_HEX_UPPER:
		case FMT_CONV_PTR:
			/* copy the preformatted number */
			/* XXX still needs width/precision logic */
			StrCopy(w, item->formatted.buffer);
			w += StrLen(w);
			break;
		case FMT_CONV_EXP_LOWER:
		case FMT_CONV_EXP_UPPER:
		case FMT_CONV_FLOAT_LOWER:
		case FMT_CONV_FLOAT_UPPER:
		case FMT_CONV_G_E_F_LOWER:
		case FMT_CONV_G_E_F_UPPER:
		case FMT_CONV_FLHEX_LOWER:
		case FMT_CONV_FLHEX_UPPER:
			/* XXX all floats still TODO */
			StrCopy(w, item->formatted.buffer);
			w += StrLen(w);
			break;
		case FMT_CONV_CHAR:
			/* copy the preformatted number */
			/* XXX still needs width logic */
			if (item->width.present) {
				for (fill = 1; fill < item->width.number; fill++) {
					*w = ' ';
					w++;
				}
			}
			*w = item->parameter.chr;
			w++;
			break;
		case FMT_CONV_STRING:
			/* XXX still needs width logic */
			use = strlen(item->parameter.str);
			if ((item->precision.present) && (item->precision.number < use))
				use = item->precision.number;
			fill = 0;
			if (item->width.present) {
				fill = item->width.number - use;
			}
			while (fill > 0) {
				*w = ' ';
				w++;
				fill--;
			}
			StrNCopy(w, item->parameter.str, use);
			w += use;
			break;
		case FMT_CONV_STORE:
			/* XXX still needs modifier cast logic */
			ival = w - *s;
			*((int *)item->parameter.ptr) = ival;
			break;
		case FMT_CONV_NONE:
		default:
			/* unhandled format */
			rc = -1;
			break;
		}

		/* skip over the complete format spec */
		p = item->fmt_conv_pos;
		p++;

		/* advance to the next parameter */
		item = item->next;
	}
	/* NUL terminate, DON'T advance the write pointer */
	if (rc == 0)
		*w = '\0';
	if (*s != NULL)
		*l = w - *s;

	/*
	 * release allocated data
	 */
	while (poslist != NULL) {
		item = poslist;
		poslist = item->next;
		item->next = NULL;

		if (item->formatted.buffer != NULL)
			free(item->formatted.buffer);
		free(item);
	}
	/* XXX release arg list in case of positional args */

	return(rc);
}

int fprintf(FILE *stream, const char *format, ...) {
	va_list args;
	char *s;
	int l;
	int rc;

	if (stream == NULL)
		return(-1);
	if (format == NULL)
		return(0);

	/* format the input into a buffer */
	va_start(args, format);
	s = NULL; l = 0;
	rc = formatted_write(format, args, &s, &l);
	va_end(args);
	if ((rc != 0) || (s == NULL)) {
		if (s != NULL)
			free(s);
		return(-1);
	}

	/* add the buffer to the file */
	rc = fwrite(s, 1, l, stream);
	free(s);
	return(rc);
}

int printf(const char *format, ...) {
	va_list args;
	char *s;
	int l;
	int rc;

	if (format == NULL)
		return(0);

	/* format the input into a buffer */
	va_start(args, format);
	s = NULL; l = 0;
	rc = formatted_write(format, args, &s, &l);
	va_end(args);
	if ((rc != 0) || (s == NULL)) {
		if (s != NULL)
			free(s);
		return(-1);
	}

	/* add the buffer to the file */
	rc = fwrite(s, 1, l, stdout_var);
	free(s);
	return(rc);
}

enum get_char_opcode {
	GCO_GET,
	GCO_UNGET,
};

union get_char_data {
	struct {
		FILE *file;
		int lastchar;
	} file;
	struct {
		const char *string;
		int index;
	} string;
};

typedef int (*get_char_callback)(enum get_char_opcode, union get_char_data *);
static int get_char_from_file(enum get_char_opcode code, union get_char_data *p) CSEC_RTL;
static int get_char_from_string(enum get_char_opcode code, union get_char_data *p) CSEC_RTL;
static int formatted_read_internal(get_char_callback cb, union get_char_data *cbdata, const char *fmt, va_list args) CSEC_RTL;
static int formatted_read_file(FILE *f, const char *fmt, va_list args) CSEC_RTL;
static int formatted_read_string(const char *s, const char *fmt, va_list args) CSEC_RTL;

static int fmt_read_number(get_char_callback cb, union get_char_data *cbdata, long long *p) CSEC_RTL;
static int fmt_read_long_int(get_char_callback cb, union get_char_data *cbdata, void *p) CSEC_RTL;

/* returns -1 on error, otherwise the number of successfully read values */
static int formatted_read_internal(get_char_callback cb, union get_char_data *cbdata, const char *fmt, va_list args) {
	/* used by Lua with "%ld" or "%lf" to parse numbers */
	/* XXX TODO (currently only handles one special case) */
	struct format_desc *poslist, *item;
	int readcount;
	int valid;

	readcount = -1; /* prepare for errors in parsing */
	poslist = NULL;

	/*
	 * XXX TODO
	 * actually this should build a list of format specs,
	 * but we fake the first entry here to fit the one case
	 */
	if (strcmp(fmt, "%ld") != 0) {
		return(readcount);
	}
	item = malloc(sizeof(*item));
	if (item == NULL) {
		return(readcount);
	}
	memset(item, 0, sizeof(*item));

	item->argidx = 1;
	item->position.number = item->argidx;
	item->flags.blanksign = 0; /* abused to skip spaces */
	item->modifier = FMT_MODI_LONG;
	item->conversion = FMT_CONV_SIGNED_INT;
	item->fmt_start_pos = fmt;
	item->fmt_conv_pos = fmt + strlen("%l");
	item->parameter.ptr = va_arg(args, long *);

	item->next = NULL;
	poslist = item;

	/* read and convert values according to the specs */
	readcount = 0;
	for (item = poslist; item != NULL; item = item->next) {

		valid = 0;

		/* space in the format spec -> skip any white space in the input */
		if (item->flags.blanksign) {
			int c;
			/* get one character */
			c = cb(GCO_GET, cbdata);
			/* and all others when it still was whitespace */
			while (isspace(c)) {
				c = cb(GCO_GET, cbdata);
			}
			/* first non whitespace char encountered */
			/* stop reading upon EOF */
			if (c == -1)
				break;
			/* bail out when unget fails */
			if (cb(GCO_UNGET, cbdata) == -1)
				break;
			/* non whitespace was put back, now go on with conversion */
		}

		/* read another value */
		switch (item->conversion) {
		case FMT_CONV_SIGNED_INT:
			/* XXX obey modifiers */
			if (fmt_read_long_int(cb, cbdata, item->parameter.ptr) == 0)
				valid = 1;
			break;
		case FMT_CONV_UNSIGNED_INT:
			/* XXX TODO all other formats */
		default:
			/* EMPTY */
			break;
		}
		if (! valid)
			break;

		/* maybe adjust according to the modifiers */
		/* XXX TODO unless already done in *_read_*() */

		/* store (according to the modifiers) */
		/* XXX TODO */

		/* only increment count after succesful storage */
		readcount++;
	}

	/* XXX release the list of format specs */
	while (poslist != NULL) {
		item = poslist;
		poslist = item->next;
		item->next = NULL;

		free(item);
	}

	/* return number of successfully converted values */
	return(readcount);
}

/* XXX these conversion routines don't handle overflows or truncation */

static int fmt_read_number(get_char_callback cb, union get_char_data *cbdata, long long *p) {
	long long ival;
	int sign;
	int rc;
	int c;

	/* preset result (zero, positive, no digits yet) */
	ival = 0;
	sign = +1;
	rc = -1;

	/* get the first character */
	c = cb(GCO_GET, cbdata);
	if (c == -1) {
		return(-1);
	}
	/* skip leading whitespace */
	while ((c == ' ') || (c == '\t')) {
		c = cb(GCO_GET, cbdata);
	}
	/* evaluate optional sign */
	if (c == '-') {
		sign = -1;
		c = cb(GCO_GET, cbdata);
	} else if (c == '+') {
		sign = +1; /* neutral, but we don't care */
		c = cb(GCO_GET, cbdata);
	}
	/* collect digits as long as possible */
	do {
		if ((c < '0') || (c > '9')) {
			if (cb(GCO_UNGET, cbdata) == -1)
				return(-1);
			break;
		}
		rc = 0; /* at least one valid digit, fine */
		ival *= 10;
		ival += c - '0';
		c = cb(GCO_GET, cbdata);
	} while (1);
	/* apply sign and store result for valid input */
	if (rc == 0) {
		ival *= sign;
		*p = ival;
	}

	return(rc);
}

static int fmt_read_long_int(get_char_callback cb, union get_char_data *cbdata, void *p) {
	long long ival;

	if (fmt_read_number(cb, cbdata, &ival) != 0)
		return(-1);
	*((long *)p) = ival;
	return(0);
}

static int get_char_from_file(enum get_char_opcode code, union get_char_data *data) {
	int c;

	if (data == NULL) {
		return(-1);
	}
	if (data->file.file == NULL) {
		return(-1);
	}
	switch (code) {
	case GCO_GET:
		c = fgetc(data->file.file);
		data->file.lastchar = c;
		break;
	case GCO_UNGET:
		c = ungetc(data->file.lastchar, data->file.file);
		break;
	}
	return(c);
}

static int formatted_read_file(FILE *f, const char *fmt, va_list args) {
	union get_char_data data = {
		.file = {
			.file = f,
			.lastchar = '\0',
		},
	};
	int rc;

	rc = formatted_read_internal(get_char_from_file, &data, fmt, args);

	return(rc);
}

/*
 * this is different than reading from a file:
 * GET stops at NUL, UNGET rewinds the read pointer
 */
static int get_char_from_string(enum get_char_opcode code, union get_char_data *data) {
	int c;

	if (data == NULL) {
		return(-1);
	}
	if (data->string.string == NULL) {
		return(-1);
	}
	switch (code) {
	case GCO_GET:
		c = data->string.string[data->string.index];
		if (c != '\0') {
			data->string.index++;
		} else {
			c = -1;
		}
		break;
	case GCO_UNGET:
		if (data->string.index > 0) {
			data->string.index--;
			c = data->string.string[data->string.index];
		} else {
			c = -1;
		}
		break;
	}
	return(c);
}

static int formatted_read_string(const char *s, const char *fmt, va_list args) {
	union get_char_data data = {
		.string = {
			.string = s,
			.index = 0,
		},
	};
	int rc;

	rc = formatted_read_internal(get_char_from_string, &data, fmt, args);

	return(rc);
}

int fscanf(FILE *stream, const char *format, ...) {
	va_list args;
	int rc;

	va_start(args, format);
	rc = formatted_read_file(stream, format, args);
	va_end(args);

	return(rc);
}

int scanf(const char *format, ...) {
	va_list args;
	int rc;

	va_start(args, format);
	rc = formatted_read_file(stdin_var, format, args);
	va_end(args);

	return(rc);
}

int sscanf(const char *s, const char *format, ...) {
	va_list args;
	int rc;

	va_start(args, format);
	rc = formatted_read_string(s, format, args);
	va_end(args);

	return(rc);
}

int fputs(const char *s, FILE *stream) {
	int rc;

	rc = fwrite(s, 1, strlen(s), stream);

	return(rc);
}

/* XXX yes, the order of fread_*() and fgets/fgetc() is "funny" here -- change that later */
static char *fread_stdin(char *s, int *size, FILE *stream, const int term, const int delim) {
	union app_event_desc ev;

	memset(s, 0, *size);

	memset(&ev, 0, sizeof(ev));
	ev.stdio.buffer = s;
	ev.stdio.length = *size;
	if (term)
		ev.stdio.length--;
	ev.stdio.delimiter = delim;
	if (stream->unget.avail) {
		ev.stdio.buffer[ev.stdio.read_count] = stream->unget.chr;
		ev.stdio.read_count++;
		stream->unget.avail = 0;
	}
	if (ev.stdio.read_count < ev.stdio.length) {
		app_add_event(APP_EVENT_STDIN, &ev);
	}
	if (ev.stdio.had_error) {
		stream->err = 1;
		return(NULL);
	}
	if (ev.stdio.had_eof) {
		stream->eof = 1;
		return(NULL);
	}
	if (term)
		s[ev.stdio.read_count] = '\0';
	*size = ev.stdio.read_count;

	return(s);
}

static char *fread_file(char *s, int size, FILE *f, const int term, const int delim) {
	/* XXX TODO */
	return(NULL);
}

static char *fread_memo(char *s, int size, FILE *f, const int term, const int delim) {
	/* XXX TODO */
	return(NULL);
}

static char *fread_fsdb(char *s, int size, FILE *f, const int term, const int delim) {
	/* XXX TODO */
	return(NULL);
}

char *fgets(char *s, int size, FILE *stream) {
	int sz;
	char *rc;

	if (s == NULL)
		return(NULL);
	if (stream == NULL)
		return(NULL);

	switch (stream->ftc) {
	case FTC_STDIN:
		sz = size;
		rc = fread_stdin(s, &sz, stream, 1, '\n');
		break;
	case FTC_STDOUT:
	case FTC_STDERR:
		rc = NULL;
		break;
	case FTC_REGFILE:
		switch (stream->storeid) {
		case FS_FILE: rc = fread_file(s, size, stream, 1, '\n'); break;
		case FS_MEMO: rc = fread_memo(s, size, stream, 1, '\n'); break;
		case FS_FSDB: rc = fread_fsdb(s, size, stream, 1, '\n'); break;
		default: rc = NULL; break;
		}
		break;
	}
	return(rc);
}

int fgetc(FILE *stream) {
	char c;
	int sz;
	char *rc;

	if (stream == NULL)
		return(-1);

	switch (stream->ftc) {
	case FTC_STDIN:
		sz = 1;
		rc = fread_stdin(&c, &sz, stream, 0, -1);
		break;
	case FTC_STDOUT:
	case FTC_STDERR:
		rc = NULL;
		break;
	case FTC_REGFILE:
		switch (stream->storeid) {
		case FS_FILE: rc = fread_file(&c, 1, stream, 0, -1); break;
		case FS_MEMO: rc = fread_memo(&c, 1, stream, 0, -1); break;
		case FS_FSDB: rc = fread_fsdb(&c, 1, stream, 0, -1); break;
		default: rc = NULL; break;
		}
		break;
	}

	if (rc == NULL)
		return(-1);
	if (sz != 1)
		return(-1);
	return(c);
}

int getc(FILE *stream) {
	return(fgetc(stream));
}

int ungetc(int c, FILE *stream) {

	if (stream == NULL)
		return(-1);
	if (stream->unget.avail)
		return(-1);

	stream->unget.chr = c;
	stream->unget.avail = 1;
	return(0);
}

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream) {
	char *s;
	int sz;
	char *rc;

	s = ptr;
	sz = size * nmemb;

	if (s == NULL)
		return(-1);
	if (stream == NULL)
		return(-1);

	switch (stream->ftc) {
	case FTC_STDIN:
		rc = fread_stdin(s, &sz, stream, 0, -1);
		break;
	case FTC_STDOUT:
	case FTC_STDERR:
		rc = NULL;
		break;
	case FTC_REGFILE:
		switch (stream->storeid) {
		case FS_FILE: rc = fread_file(s, size, stream, 0, -1); break;
		case FS_MEMO: rc = fread_memo(s, size, stream, 0, -1); break;
		case FS_FSDB: rc = fread_fsdb(s, size, stream, 0, -1); break;
		default: rc = NULL; break;
		}
		break;
	}

	if (rc == NULL) {
		return(-1);
	}

	sz /= size;
	return(sz);
}

static size_t fwrite_file(const void *ptr, size_t size, size_t nmemb, FILE *stream) {
	/* XXX TODO */
	return(-1);
}

static size_t fwrite_memo(const void *ptr, size_t size, size_t nmemb, FILE *stream) {
	/* XXX TODO */
	return(-1);
}

static size_t fwrite_fsdb(const void *ptr, size_t size, size_t nmemb, FILE *stream) {
	/* XXX TODO */
	return(-1);
}

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream) {
	char *s;
	int sz;
	enum app_event_code code;
	union app_event_desc ev;
	int rc;

	s = (char *)ptr; /* UNCONST */
	sz = size * nmemb;

	switch (stream->ftc) {
	case FTC_STDIN:
		rc = -1;
		break;
	case FTC_STDOUT:
	case FTC_STDERR:
		if (stream->ftc == FTC_STDOUT)
			code = APP_EVENT_STDOUT;
		else if (stream->ftc == FTC_STDERR)
			code = APP_EVENT_STDERR;
		memset(&ev, 0, sizeof(ev));
		ev.stdio.buffer = s;
		ev.stdio.length = sz;
		app_add_event(code, &ev);
		if (ev.stdio.had_error) {
			stream->err = 1;
			rc = -1;
			break;
		}
		rc = sz / size;
		break;
	case FTC_REGFILE:
		switch (stream->storeid) {
		case FS_FILE: rc = fwrite_file(ptr, size, nmemb, stream); break;
		case FS_MEMO: rc = fwrite_memo(ptr, size, nmemb, stream); break;
		case FS_FSDB: rc = fwrite_fsdb(ptr, size, nmemb, stream); break;
		default: rc = -1; break;
		}
		break;
	}

	return(rc);
}

int feof(FILE *stream) {
	if (stream == NULL)
		return(0);
	return(stream->eof ? 1 : 0);
}

int ferror(FILE *stream) {
	if (stream == NULL)
		return(0);
	return(stream->err ? 1 : 0);
}

void clearerr(FILE *stream) {
	if (stream == NULL)
		return;

	stream->err = 0;
	stream->eof = 0;
	return;
}

FILE *tmpfile (void) {
	/* TODO */
  dbg();
	/* used in io.tmpfile() */
	return(NULL);
}

int fseek_file(FILE *stream, long offset, int whence) {
	/* XXX TODO */
	return(-1);
}

int fseek_memo(FILE *stream, long offset, int whence) {
	/* XXX TODO */
	return(-1);
}

int fseek_fsdb(FILE *stream, long offset, int whence) {
	/* XXX TODO */
	return(-1);
}

int fseek(FILE *stream, long offset, int whence) {
	/* used in io.seek() */
	int rc;

	if (stream == NULL)
		return(-1);
	switch (stream->ftc) {
	case FTC_STDIN:
	case FTC_STDOUT:
	case FTC_STDERR:
		/* not seekable */
		rc = -1;
		break;
	case FTC_REGFILE:
		switch (stream->storeid) {
		case FS_FILE: rc = fseek_file(stream, offset, whence); break;
		case FS_MEMO: rc = fseek_memo(stream, offset, whence); break;
		case FS_FSDB: rc = fseek_fsdb(stream, offset, whence); break;
		default: rc = -1; break;
		}
		break;
	}

	return(rc);
}

long ftell_file(FILE *stream) {
	/* TODO */
	return(-1);
}

long ftell_memo(FILE *stream) {
	/* TODO */
	return(-1);
}

long ftell_fsdb(FILE *stream) {
	/* TODO */
	return(-1);
}

long ftell(FILE *stream) {
	/* used in io.seek() */
	long rc;

	if (stream == NULL)
		return(-1);
	switch (stream->ftc) {
	case FTC_STDIN:
	case FTC_STDOUT:
	case FTC_STDERR:
		/* not seekable, no "current position" */
		rc = -1;
		break;
	case FTC_REGFILE:
		switch (stream->storeid) {
		case FS_FILE: rc = ftell_file(stream); break;
		case FS_MEMO: rc = ftell_memo(stream); break;
		case FS_FSDB: rc = ftell_fsdb(stream); break;
		default: rc = -1; break;
		}
		break;
	}

	return(rc);
}

int setvbuf(FILE *stream, char *buf, int mode , size_t size) {
	/* TODO */
	/* used in io.setvbuf() */
	return(-1);
}

int fflush_file(FILE *f) {
	/* XXX TODO */
	return(-1);
}

int fflush_memo(FILE *f) {
	/* XXX TODO */
	return(-1);
}

int fflush_fsdb(FILE *f) {
	/* XXX TODO */
	return(-1);
}

int fflush(FILE *stream) {
	/* used in io.flush() and in the lua(1) program */
	int rc;

	if (stream == NULL)
		return(-1);
	switch (stream->ftc) {
	case FTC_STDIN:
		/* read only, nothing to flush */
		rc = -1;
		break;
	case FTC_STDOUT:
	case FTC_STDERR:
		/* unbuffered, nothing to do */
		rc = 0;
		break;
	case FTC_REGFILE:
		switch (stream->storeid) {
		case FS_FILE: rc = fflush_file(stream); break;
		case FS_MEMO: rc = fflush_memo(stream); break;
		case FS_FSDB: rc = fflush_fsdb(stream); break;
		default: rc = -1; break;
		}
		break;
	}

	return(rc);
}

static int remove_file(const char *pathname, const char *filename) {
	FILE *f;
	UInt16 cardno;

	f = fopen(pathname, "r");
	if (f == NULL)
		return(-1);

	cardno = f->store.file.cardno;
	fclose(f);

	FileDelete(cardno, filename);
	return(0);
}

static int remove_memo(const char *pathname, const char *filename) {
	FILE *f;

	/* XXX TODO */
	f = fopen(pathname, "r");
	if (f == NULL)
		return(-1);

	DmDeleteRecord(f->store.memo.dbp, f->store.memo.idx);
	fclose(f);
	return(0);
}

static int remove_fsdb(const char *pathname, const char *filename) {
	/* XXX TODO */
	return(-1);
}

/*
 * XXX TODO
 * insist in a prefix or default to one storage,
 * don't try to guess or even search the file
 */
int remove(const char *pathname) {
	const char *fn;
	enum file_store id;
	int rc;

	if (pathname == NULL)
		return(-1);

	fn = pathname;
	id = FS_NONE;
	rc = 0;

	/* get the file type from its name prefix */
#define FS_CHECK_PREFIX(name, type) \
	if (StrNCompare(fn, "//" ## name ## "/", strlen("//" ## name ## "/")) == 0) { \
		fn += strlen("//" ## name); \
		id = type; \
	}
	FS_CHECK_PREFIX("file", FS_FILE);
	FS_CHECK_PREFIX("memo", FS_MEMO);
	FS_CHECK_PREFIX("fsdb", FS_FSDB);
#undef FS_CHECK_PREFIX

	/* don't search everywhere, just default to one protocol */
	if (id == FS_NONE) {
		id = FS_FILE;
	}

	/* call the remove() routine for the storage type */
	switch (id) {
	case FS_FILE: rc = remove_file(pathname, fn); break;
	case FS_MEMO: rc = remove_memo(pathname, fn); break;
	case FS_FSDB: rc = remove_fsdb(pathname, fn); break;
	default: rc = -1; break;
	}
	return(rc);
}

static int rename_file(const char *oldpath, const char *oldfn, const char *newpath, const char *newfn) {
	/* TODO */
	return(-1);
}

static int rename_memo(const char *oldpath, const char *oldfn, const char *newpath, const char *newfn) {
	/* TODO */
	return(-1);
}

static int rename_fsdb(const char *oldpath, const char *oldfn, const char *newpath, const char *newfn) {
	/* TODO */
	return(-1);
}

int rename(const char *oldpath, const char *newpath) {
	int rc;
	enum file_store oldid, newid;
	const char *oldfn, *newfn;

	if ((oldpath == NULL) || (newpath == NULL))
		return(-1);

	oldfn = oldpath; oldid = FS_NONE;
	newfn = newpath; newid = FS_NONE;

	/* determine the store id from the origin, default to "file" */
#define FS_CHECK_PREFIX(name, type) \
	if (StrNCompare(oldfn, "//" ## name ## "/", strlen("//" ## name ## "/")) == 0) { \
		oldfn += strlen("//" ## name); \
		oldid = type; \
	}
	FS_CHECK_PREFIX("file", FS_FILE);
	FS_CHECK_PREFIX("memo", FS_MEMO);
	FS_CHECK_PREFIX("fsdb", FS_FSDB);
#undef FS_CHECK_PREFIX
	if (oldid == FS_NONE) {
		oldid = FS_FILE;
	}

	/* determine the store id from the target */
#define FS_CHECK_PREFIX(name, type) \
	if (StrNCompare(newfn, "//" ## name ## "/", strlen("//" ## name ## "/")) == 0) { \
		newfn += strlen("//" ## name); \
		newid = type; \
	}
	FS_CHECK_PREFIX("file", FS_FILE);
	FS_CHECK_PREFIX("memo", FS_MEMO);
	FS_CHECK_PREFIX("fsdb", FS_FSDB);
#undef FS_CHECK_PREFIX
	/* refuse to rename between different storage types */
	if ((newid != FS_NONE) && (newid != oldid)) {
		return(-1);
	}

	switch (oldid) {
	case FS_FILE: rc = rename_file(oldpath, oldfn, newpath, newfn); break;
	case FS_MEMO: rc = rename_memo(oldpath, oldfn, newpath, newfn); break;
	case FS_FSDB: rc = rename_fsdb(oldpath, oldfn, newpath, newfn); break;
	default: rc = -1; break;
	}

	return(rc);
}

char *tmpnam(char *s) {
	/* used in os.tmpname() when mkstemp() is not available (see luaconf.h) */
	static int count = 0;
	static char buffer[L_tmpnam]; /* is that 20? or 32? */

	char *rc;

	/* generate a new "unique" name in our buffer */
	count++;
	/* XXX BEWARE! make sure the string fits into the buffer */
	StrCopy(buffer, "//file/tmp-");
	StrIToA(buffer + StrLen(buffer), count);

	/* copy the name into the application space when specified */
	if (s != NULL) {
		StrCopy(s, buffer);
		rc = s;
	} else {
		rc = buffer;
	}

	return(rc);
}

FILE *get_stdio_stream(const int n) {
	switch (n) {
	case 0: return(stdin_var);
	case 1: return(stdout_var);
	case 2: return(stderr_var);
	default: return(NULL);
	}
}

/* XXX TODO */
int snprintf(char *str, const size_t size, const char *format, ...) {
	va_list args;
	char *s;
	int l;
	int rc;

	if (str == NULL)
		return(-1);
	if (size <= 0)
		return(-1);
	if (format == NULL)
		return(0);

	/* format the input into a buffer */
	va_start(args, format);
	s = NULL; l = 0;
	rc = formatted_write(format, args, &s, &l);
	va_end(args);
	if ((rc != 0) || (s == NULL)) {
		if (s != NULL)
			free(s);
		return(-1);
	}

	/*
	 * use as much of the output as fits into the buffer,
	 * make sure to NUL terminate the returned data
	 */
	if (l >= size)
		l = size - 1;
	MemMove(str, s, l);
	str[l] = '\0';
	free(s);
	return(rc);
}

int sprintf(char *str, const char *format, ...) {
	va_list args;
	char *s;
	int l;
	int rc;

	if (str == NULL)
		return(-1);
	if (format == NULL)
		return(-1);

	/* format the input into a buffer */
	va_start(args, format);
	s = NULL; l = 0;
	rc = formatted_write(format, args, &s, &l);
	va_end(args);
	if ((rc != 0) || (s == NULL)) {
		if (s != NULL)
			free(s);
		return(-1);
	}

	MemMove(str, s, l);
	str[l] = '\0';
	free(s);
	return(rc);
}

/* XXX shall this done by fopen()? */
static int init_fileio(void) {
	FILE *f;

	/* create stdin */
	f = malloc(sizeof(*f));
	if (f == NULL) {
		return(-1);
	}
	memset(f, 0, sizeof(*f));
	f->ftc = FTC_STDIN;
	f->mode = FM_READ;
	stdin_var = f;

	/* create stdout */
	f = malloc(sizeof(*f));
	if (f == NULL) {
		fclose(stdin_var);
		return(-1);
	}
	memset(f, 0, sizeof(*f));
	f->ftc = FTC_STDOUT;
	f->mode = FM_WRITE | FM_APPEND;
	stdout_var = f;

	/* create stderr */
	f = malloc(sizeof(*f));
	if (f == NULL) {
		fclose(stdin_var);
		fclose(stdout_var);
		return(-1);
	}
	memset(f, 0, sizeof(*f));
	f->ftc = FTC_STDERR;
	f->mode = FM_WRITE | FM_APPEND;
	stderr_var = f;

	return(0);
}

static void done_fileio(void) {
	/* XXX TODO close all open files, remove tempfiles */

	fclose(stdin_var);
	fclose(stdout_var);
	fclose(stderr_var);

	return;
}

/* } */

/*
 * math.h
 */
/* { */
/* the "convenience wrapper" was released into the public domain */
#include "../contrib/MathLib/MathLib.c"
/* } */


/*
 * time.h
 */
/* { */
clock_t clock(void) {
	UInt32 ticks;

	ticks = TimGetTicks();

	return(ticks);
}

struct tm *gmtime(const time_t *timep) {
	static struct tm tmbuff;
	DateTimePtr tmp;

	UInt32 ticks;

	if (timep == NULL)
		return(NULL);

	ticks = *timep;
	tmp = MemPtrNew(sizeof(*tmp));
	if (tmp == NULL)
		return(NULL);
	TimSecondsToDateTime(ticks, tmp);
	tmbuff.tm_year = tmp->year;
	tmbuff.tm_mon  = tmp->month;
	tmbuff.tm_mday = tmp->day;
	tmbuff.tm_hour = tmp->hour;
	tmbuff.tm_min  = tmp->minute;
	tmbuff.tm_sec  = tmp->second;
	MemPtrFree(tmp);

	tmbuff.tm_year -= 1900;
	tmbuff.tm_mon -= 1;

	return(&tmbuff);
}

struct tm *localtime(const time_t *timep) {
	/* should be Good Enough(TM), LmLocaleType was introduced in PalmOS 4.0 */
	return(gmtime(timep));
}

size_t strftime(char *s, size_t max, const char *format, const struct tm *tm) {
	char *ptr;
	size_t rem;
	const char *fmt;
	DateTimeType t;

	if (s == NULL)
		return(0);
	if (max <= 0)
		return(0);
	if ((format == NULL) || (*format == '\0'))
		return(0);
	if (tm == NULL)
		return(0);
	MemSet(s, max, 0);

	/* copy input data, position to buffer's start */
	ptr = s;
	rem = max - 1;
	fmt = format;
	MemSet(&t, sizeof(t), 0);
	t.year   = tm->tm_year + 1900;
	t.month  = tm->tm_mon + 1;
	t.day    = tm->tm_mday;
	t.hour   = tm->tm_hour;
	t.minute = tm->tm_min;
	t.second = tm->tm_sec;

	/* as long as the format and the result buffer are not exhausted ... */
	while ((*fmt != '\0') && (rem >= 1)) {
		char *dfmt, *tfmt, *sfmt;
		char char_subst;
		int modi_e, modi_o;

		/* copy any non format text */
		if (*fmt != '%') {
			*ptr = *fmt;
			ptr++; rem--;
			fmt++;
			continue;
		}
		/* format found, determine kind */
		fmt++;

		/* simple character replacement? */
		char_subst = '\0';
		switch (*fmt) {
		case 'n': char_subst = '\n'; break;
		case 't': char_subst = '\t'; break;
		case '%': char_subst = '%'; break;
		default: /* EMPTY */; break;
		}
		if (char_subst != '\0') {
			*ptr = char_subst;
			ptr++; rem--;
			fmt++;
			continue;
		}

		/* detect (but don't support) the 'E' and 'O' modifiers */
		modi_e = 0;
		modi_o = 0;
		if (*fmt == 'E') {
			modi_e = 1;
			fmt++;
		} else if (*fmt == 'O') {
			modi_o = 1;
			fmt++;
		}
		/* bail out on incomplete format specs */
		if (*fmt == '\0')
			break;

		/*
		 * map supported formats to PalmOS format strings
		 * (works for date types), or register to process
		 * (day)time formats, or register for special logic
		 */
		dfmt = NULL; tfmt = NULL; sfmt = NULL;
		switch (*fmt) {
		case 'a': dfmt = "^1s"; break;
		case 'A': dfmt = "^1l"; break;
		case 'b': dfmt = "^3s"; break;
		case 'B': dfmt = "^3l"; break;
		case 'c': sfmt = "c"; break;
		case 'C': sfmt = "C"; break;
		case 'd': dfmt = "^0z"; break;
		case 'D': dfmt = "^0s/^2s/^4s"; break;
		case 'e': dfmt = "^0s"; break; /* XXX not completely correct */
		case 'F': dfmt = "^4l-^2z-^0z"; break;
		case 'h': dfmt = "^3s"; break;
		case 'H': tfmt = "H"; break;
		case 'I': tfmt = "I"; break;
		case 'j': sfmt = "j"; break;
		case 'k': tfmt = "k"; break;
		case 'l': tfmt = "l"; break;
		case 'm': dfmt = "^2z"; break;
		case 'M': tfmt = "M"; break;
		case 'n': /* EMPTY */; break; /* should not happen, caught above */
		case 'p': tfmt = "p"; break;
		case 'P': tfmt = "P"; break;
		case 'r': tfmt = "r"; break;
		case 'R': tfmt = "R"; break;
		case 's': sfmt = "s"; break;
		case 'S': tfmt = "S"; break;
		case 't': /* EMPTY */; break; /* should not happen, caught above */
		case 'T': tfmt = "T"; break;
		case 'u': sfmt = "u"; break;
		case 'w': sfmt = "w"; break;
		case 'x': sfmt = "x"; break;
		case 'X': sfmt = "X"; break;
		case 'y': dfmt = "^4s"; break;
		case 'Y': dfmt = "^4r"; break;
		/* XXX handle more format specs */
		case 'g': /* NOIMPL */; break;
		case 'G': /* NOIMPL */; break;
		case 'U': /* NOIMPL */; break;
		case 'V': /* NOIMPL */; break;
		case 'W': /* NOIMPL */; break;
		case 'z': /* NOIMPL */; break; /* would require PalmOS 4 */
		case 'Z': /* NOIMPL */; break; /* would require PalmOS 4 */
		case '+': /* NOIMPL */; break;
		default: /* EMPTY */; break;
		}

		/* get verbatim copy for unsupported formats */
		if ((dfmt == NULL) && (tfmt == NULL) && (sfmt == NULL)) {
			*ptr = *fmt;
			ptr++; rem--;
			fmt++;
			continue;
		}
		/* else: append translation to the result buffer */

		/* handle PalmOS style date format specs */
		if (dfmt != NULL) {
			DateTemplateToAscii(dfmt, t.month, t.day, t.year, ptr, rem);
			rem -= StrLen(ptr);
			ptr += StrLen(ptr);
			fmt++;
			continue;
		}

		/* handle daytime formats (no PalmOS support) */
		if (tfmt != NULL) {
			char tb[timeStringLength];
			if (StrCompare(tfmt, "H") == 0) {
				if (rem < 2)
					break;
				TimeToAscii(t.hour, t.minute, false, tb);
				StrNCopy(ptr, tb, 2);
			} else if (StrCompare(tfmt, "I") == 0) {
				if (rem < 2)
					break;
				TimeToAscii(t.hour, t.minute, true, tb);
				StrNCopy(ptr, tb + 3, 2);
			} else if (StrCompare(tfmt, "k") == 0) {
				if (rem < 2)
					break;
				TimeToAscii(t.hour, t.minute, false, tb);
				if (tb[0] == '0')
					tb[0] = ' ';
				StrNCopy(ptr, tb, 2);
			} else if (StrCompare(tfmt, "l") == 0) {
				if (rem < 2)
					break;
				TimeToAscii(t.hour, t.minute, true, tb);
				if (tb[0] == '0')
					tb[0] = ' ';
				StrNCopy(ptr, tb, 2);
			} else if (StrCompare(tfmt, "M") == 0) {
				if (rem < 2)
					break;
				TimeToAscii(t.hour, t.minute, false, tb);
				StrNCopy(ptr, tb + 3, 2);
			} else if (StrCompare(tfmt, "p") == 0) {
				if (rem < 2)
					break;
				TimeToAscii(t.hour, t.minute, true, tb);
				StrNCopy(ptr, tb + 6, 2);
				/* StrToUpper(ptr); */ /* missing on PalmOS */
			} else if (StrCompare(tfmt, "P") == 0) {
				if (rem < 2)
					break;
				TimeToAscii(t.hour, t.minute, true, tb);
				StrNCopy(ptr, tb + 6, 2);
			} else if (StrCompare(tfmt, "r") == 0) {
				char tb2[timeStringLength];
				TimeToAscii(0, t.second, false, tb2);
				TimeToAscii(t.hour, t.minute, true, tb);
				if (rem < StrLen(tb) + 3)
					break;
				StrNCopy(ptr, tb, 5);
				StrNCopy(ptr, tb2 + 2, 3);
				StrCopy(ptr, tb + 5);
			} else if (StrCompare(tfmt, "R") == 0) {
				TimeToAscii(t.hour, t.minute, false, tb);
				if (rem < StrLen(tb))
					break;
				StrCopy(ptr, tb);
			} else if (StrCompare(tfmt, "S") == 0) {
				if (rem < 2)
					break;
				TimeToAscii(t.hour, t.second, false, tb);
				StrNCopy(ptr, tb + 3, 2);
			} else if (StrCompare(tfmt, "T") == 0) {
				char tb2[timeStringLength];
				TimeToAscii(0, t.second, false, tb2);
				TimeToAscii(t.hour, t.minute, false, tb);
				if (rem < StrLen(tb) + 3)
					break;
				StrNCopy(ptr, tb, 5);
				StrNCopy(ptr, tb2 + 2, 3);
			} else {
				/* setup above and not handled here? */
				/* XXX this is a programmer's error! */
				dbg();
				break;
			}
			rem -= StrLen(ptr);
			ptr += StrLen(ptr);
			fmt++;
			continue;
		}

		/* handle special formats */
		if (sfmt != NULL) {
			if (StrCompare(sfmt, "c") == 0) {
				DateFormatType dft;
				char db[longDateStrLength];
				char tb[timeStringLength];
				dft = PrefGetPreference(prefLongDateFormat);
				DateToAscii(t.month, t.day, t.year, dft, db);
				TimeToAscii(t.hour, t.minute, false, tb);
				if (rem < StrLen(db) + 1 + StrLen(tb))
					break;
				StrCopy(ptr, db);
				rem -= StrLen(ptr);
				ptr += StrLen(ptr);
				*ptr = ' ';
				ptr++;
				rem--;
				StrCopy(ptr, tb);
			} else if (StrCompare(sfmt, "C") == 0) {
				char cb[5];
				if (rem < 2)
					break;
				MemSet(cb, sizeof(cb), 0);
				DateTemplateToAscii("^4z", t.month, t.day, t.year, cb, 5);
				if (StrLen(cb) != 4)
					break;
				StrNCopy(ptr, cb + 2, 2);
			} else if (StrCompare(sfmt, "D") == 0) {
				/* XXX %m/%d/%y for Americans */
				char cb[12];
				if (rem < 10)
					break;
				MemSet(cb, sizeof(cb), 0);
				DateTemplateToAscii("^2/^0/^4", t.month, t.day, t.year, cb, 12);
				if (StrLen(cb) > 10)
					break;
				StrCopy(ptr, cb);
			} else if (StrCompare(sfmt, "j") == 0) {
				/* XXX day of the year, 001 to 366 */
			} else if (StrCompare(sfmt, "s") == 0) {
				/* XXX seconds since the Epoch */
				if (rem < 10)
					break;
				/* time(NULL); snprintf(); */
			} else if (StrCompare(sfmt, "u") == 0) {
				char cb[2];
				if (rem < 1)
					break;
				MemSet(cb, sizeof(cb), 0);
				DateTemplateToAscii("^1", t.month, t.day, t.year, cb, 2);
				if (StrLen(cb) != 1)
					break;
				if (cb[0] == '0')
					cb[0] = '7';
				*ptr = cb[0];
			} else if (StrCompare(sfmt, "w") == 0) {
				char cb[2];
				if (rem < 1)
					break;
				MemSet(cb, sizeof(cb), 0);
				DateTemplateToAscii("^1", t.month, t.day, t.year, cb, 2);
				if (StrLen(cb) != 1)
					break;
				if (cb[0] == '7')
					cb[0] = '0';
				*ptr = cb[0];
			} else if (StrCompare(sfmt, "x") == 0) {
				char db[longDateStrLength];
				DateToAscii(t.month, t.day, t.year, PrefGetPreference(prefDateFormat), db);
				if (rem < StrLen(db))
					break;
				StrCopy(ptr, db);
			} else if (StrCompare(sfmt, "X") == 0) {
				char tb[timeStringLength];
				TimeToAscii(t.hour, t.minute, false, tb); /* pref? */
				if (rem < StrLen(tb))
					break;
				StrCopy(ptr, tb);
			} else {
				/* setup above and not handled here? */
				/* XXX this is a programmer's error! */
				dbg();
				break;
			}
			rem -= StrLen(ptr);
			ptr += StrLen(ptr);
			fmt++;
			continue;
		}

		/* UNREACH */

	} while (1);

	*ptr = '\0';
	return(ptr - s);
}

time_t time(time_t *t) {
	UInt32 ticks;

	ticks = TimGetSeconds();
	if (t != NULL)
		*t = ticks;
	return(ticks);
}

time_t mktime(struct tm *tm) {
	DateTimePtr tmp;
	UInt32 ticks;

	if (tm == NULL)
		return(-1);

	tmp = MemPtrNew(sizeof(*tmp));
	if (tmp == NULL)
		return(-1);
	MemSet(tmp, sizeof(*tmp), 0);

	tmp->year   = tm->tm_year; tmp->year += 1900;
	tmp->month  = tm->tm_mon; tmp->month += 1;
	tmp->day    = tm->tm_mday;
	tmp->hour   = tm->tm_hour;
	tmp->minute = tm->tm_min;
	tmp->second = tm->tm_sec;

	ticks = TimDateTimeToSeconds(tmp);

	MemPtrFree(tmp);

	return(ticks);
}

time_t difftime(time_t time1, time_t time0) {
	return(time1 - time0);
}
/* } */


/*
 * signal.h
 */
/* used in lua.c */
/* { */
sighandler_t signal(int signum, sighandler_t handler) {
	return(SIG_ERR);
}
/* } */


/*
 * assert.h
 */
/* used in lua.c and in application code */
/* { */

/* "boolean" return code, 0 for "does not apply", 1 for "is OK, applies" */
int assert_int(int cond, const char *txt) {

	/* normalize bool representation */
	cond = (cond) ? 1 : 0;

	/* pop up an alert upon failure */
	if (! cond) {
		FrmCustomAlert(2301 /* XXX hardcoded ID */, "assertion failed", txt, NULL);
		dbg();
	}

	/* return boolean */
	return(cond);
}

/* } */


/*
 * "OS base" initialization and shutdown,
 * called from the application before any other routine
 */
/* { */

enum glueinit_retcode osglue_init(const UInt32 creator) {
	Err err;

	/* store the PalmOS app creator code for file/DB ops */
	CREATOR_CODE = creator;

	/* prepare memory bookkeeping */
	if (memory_prepare() != 0)
		return(GLUEINIT_MEM);

	/* load the MathLib library */
	err = SysLibFind(MathLibName, &MathLibRef);
	if (err != 0)
		err = SysLibLoad(LibType, MathLibCreator, &MathLibRef);
	if (err != 0)
		return(GLUEINIT_LIB);
	err = MathLibOpen(MathLibRef, MathLibVersion);
	if (err != 0)
		return(GLUEINIT_LIB);

	/* prepare file I/O */
	err = init_fileio();
	if (err != 0)
		return(GLUEINIT_FILE);

	/* OK, successfully done */
	return(GLUEINIT_OK);
}

int osglue_done(void) {
	Err err;
	UInt16 numapps;

	/* cleanup file I/O */
	done_fileio();

	/* release the MathLib */
	if (MathLibRef != sysInvalidRefNum) {
		err = MathLibClose(MathLibRef, &numapps);
		if ((err == 0) && (numapps == 0))
			SysLibRemove(MathLibRef);
	}

	/* cleanup memory pool */
	memory_release();

	return(0);
}

/* } */

/* ----- E O F ----------------------------------------------- */
