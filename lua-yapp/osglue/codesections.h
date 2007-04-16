/* ----- codesections.h -------------------------------------- */
/*
 * assign routines to code segments to circumvent the
 * 64KB size limit of PalmOS DB records (code in .prc)
 * (plus a 32KB jump distance limitation of the CPU)
 */

#ifndef CODE_SECTIONS_H
#define CODE_SECTIONS_H


/*
 * declare code sections,
 * use macro names according to the source file,
 * the section name need not be the same but
 * MUST NOT exceed an eight characters length
 * (that's a limit in the COFF executable format),
 * all sections must be listed in the .def file and
 * what's listed in the .def file needs to get accessed
 * from the executable to satisfy the linker
 */

#if defined(__palmos__)
  #define CSEC(x) __attribute__((section("cs_" #x)))
#else
  #define CSEC(x) /* EMPTY */
#endif

/* "none" aka default section */
#define CSEC_NONE	/* EMPTY */

/* "catch all" */
#define CSEC_LUA	CSEC(lua)

/* liblua.a */
#define CSEC_LAPI	CSEC(lapi)
#define CSEC_LAUXLIB	CSEC(lauxl)
#define CSEC_LBASELIB	CSEC(lbasl)
#define CSEC_LCODE	CSEC(lcode)
#define CSEC_LDBLIB	CSEC(ldblb)
#define CSEC_LDEBUG	CSEC(lcode)
#define CSEC_LDO	CSEC(ldo)
#define CSEC_LDUMP	CSEC(ldump)
#define CSEC_LFUNC	CSEC(lfunc)
#define CSEC_LGC	CSEC(lgc)
#define CSEC_LINIT	CSEC(linit)
#define CSEC_LIOLIB	CSEC(liolb)
#define CSEC_LLEX	CSEC(llex)
#define CSEC_LMATHLIB	CSEC(lmath)
#define CSEC_LMEM	CSEC(lmem)
#define CSEC_LOADLIB	CSEC(ldlib)
#define CSEC_LOBJECT	CSEC(lobj)
#define CSEC_LOPCODES	CSEC(lcode)
#define CSEC_LOSLIB	CSEC(loslb)
#define CSEC_LPARSER	CSEC(lprsr)
#define CSEC_LSTATE	CSEC(lstat)
#define CSEC_LSTRING	CSEC(lstr)
#define CSEC_LSTRLIB	CSEC(lstrl)
#define CSEC_LTABLE	CSEC(ltbl)
#define CSEC_LTABLIB	CSEC(ltblb)
#define CSEC_LTM	CSEC(ltm)
#define CSEC_LUAC	CSEC(luac)
#define CSEC_LUNDUMP	CSEC(ldump)
#define CSEC_LVM	CSEC(lvm)
#define CSEC_LZIO	CSEC(lzio)
#define CSEC_LPRINT	CSEC(lprnt)

/* the runtime library */
#define CSEC_RTL	CSEC(rtl)
#define CSEC_MATHRTL	CSEC(martl)

/* the app and PalmOS library */
#define CSEC_APP	CSEC(app)
#define CSEC_PLIB	CSEC(plib)

/*
 * a routine which references all sections at once
 * (to make the linker happy)
 */
void code_sections_reference(void) CSEC_APP;

#endif /* CODE_SECTIONS_H */

/* ----- E O F ----------------------------------------------- */
