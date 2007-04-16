/* ----- palmglueinject.h ------------------------------------ */
/*
 * the PalmOS application may "inject" data into the "OS runtime"
 * (like keyboard input, timer events, or something)
 */

#ifndef PALMGLUEINJECT_H
#define PALMGLUEINJECT_H

enum glueinit_retcode {
	GLUEINIT_OK,
	GLUEINIT_MEM,
	GLUEINIT_LIB,
	GLUEINIT_FILE,
};

enum glueinit_retcode osglue_init(const UInt32 creator) CSEC_RTL;
int osglue_done(void) CSEC_RTL;

#endif /* PALMGLUEINJECT_H */

/* ----- E O F ----------------------------------------------- */
