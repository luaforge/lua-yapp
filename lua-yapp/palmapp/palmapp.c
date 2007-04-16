/* ----- palmapp.c ------------------------------------------- */

/* PalmOS SDK includes */
#include <PalmOS.h>

/* OS "emulation"/compat stuff (needed before Lua) */
#include "codesections.h"
#include "oswrapper.h"
#include "osglueinject.h"
#include "palmappinject.h"

/* Lua related includes */
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <llimits.h>
#include <lopcodes.h>
#include <lundump.h>

/* UI declarations, resources */
#include "palmapp.h"

/* Lua libraries */
#include "libpalmos.h"

#include <string.h>


#if 0
/* desparate try to force global vars into the default data section */
static void touch_global_vars(void) CSEC_NONE;
static void touch_global_vars(void) {
	const void *p;
	p = luaP_opmodes;
	p = luaP_opnames;
}
#endif


/* code segment forward declarations */
enum output_chunk_type;
struct inout_desc;

/* declared here in full to avoid build warnings */
enum output_chunk_type {
	OCT_TERM,
	OCT_HEAD,
	OCT_STDIN,
	OCT_STDOUT,
	OCT_STDERR,
};

static Err StartApplication(void) CSEC_APP;
static void StopApplication(void) CSEC_APP;
static Err OpenSourceDatabase(void) CSEC_APP;
static void CloseSourceDatabase(void) CSEC_APP;
static MemHandle FetchSourceDatabase(void) CSEC_APP;
static void StoreSourceDatabase(MemHandle hdl) CSEC_APP;
static struct inout_desc *inout_getdata(void) CSEC_APP;
static void *inout_getgadget(FormPtr *frm, UInt16 *idx) CSEC_APP;
static MemHandle output_getmemhandle(void) CSEC_APP;
static MemHandle input_getmemhandle(void) CSEC_APP;
static int output_verify_layout(void) CSEC_APP;
static void output_refreshdisplay(void) CSEC_APP;
static MemHandle output_init(void) CSEC_APP;
static void output_done(MemHandle hdl) CSEC_APP;
static void output_clear(void) CSEC_APP;
static MemHandle input_init(void) CSEC_APP;
static void input_done(MemHandle hdl) CSEC_APP;
static int output_addchunk(MemHandle hdl, enum output_chunk_type type, UInt8 *data, UInt32 size) CSEC_APP;
static int output_dump(const int optimize) CSEC_APP;
static void app_output_show(const enum app_event_code code, union app_event_desc *data) CSEC_APP;
static Boolean app_input_callback(Int16 btn, Char *txt) CSEC_APP;
static void app_input_get(const enum app_event_code code, union app_event_desc *data) CSEC_APP;
static void DrawOutputGadgetData(void) CSEC_APP;
static void DrawOutputGadget(void) CSEC_APP;
static Boolean frmMainHandleGadget(FormGadgetTypeInCallback *g, UInt16 cmd, void *para) CSEC_APP;
static void InitMainForm(FormPtr form) CSEC_APP;
static void DoneMainForm(FormPtr form) CSEC_APP;
static void script_add_string(FieldPtr fld, const char *txt, const int len) CSEC_APP;
static Boolean frmMainHandleEvent(EventPtr event) CSEC_APP;
static void InitSetupForm(FormPtr form) CSEC_APP;
static void SaveSetupForm(FormPtr form, Boolean *bClear, Boolean *bRedraw) CSEC_APP;
static void DoneSetupForm(FormPtr form) CSEC_APP;
static void runtime_open_func(void) CSEC_APP;
static void InitRuntimeForm(FormPtr form) CSEC_APP;
static void DoneRuntimeForm(FormPtr form) CSEC_APP;
static Boolean frmRuntimeHandleEvent(EventPtr event) CSEC_APP;
static Boolean ApplicationHandleEvent(EventPtr event) CSEC_APP;
static int BytecodeScript(const char *script) CSEC_APP;
static int RunScript(const char *script) CSEC_APP;
static void SetFieldTextFromHandle(FieldPtr field, MemHandle newHandle, Boolean redraw) CSEC_APP;
static void UpdateScrollbar(FormPtr form, UInt16 fieldID, UInt16 scrollbarID) CSEC_APP;
static void ScrollLines(FormPtr form, UInt16 fieldID, UInt16 scrollbarID, Int16 numLinesToScroll, Boolean redraw) CSEC_APP;
static void PageScroll(FormPtr form, UInt16 fieldID, UInt16 scrollbarID, WinDirectionType direction) CSEC_APP;
static void InitMemoList(void) CSEC_APP;
static void DoneMemoList(void) CSEC_APP;
static int LoadStringResource(UInt16 id, FieldPtr fld) CSEC_APP;
static int LoadMemoItem(const UInt16 recno, FieldPtr fld) CSEC_APP;
static int SaveMemoItem(UInt16 recno, const Boolean mknew, FieldPtr fld) CSEC_APP;
static void DrawMemoListItem(Int16 item, RectangleType *bounds, Char **itemsTxt) CSEC_APP;
static UInt16 RunMemoSelect(FormPtr form, int *mknew) CSEC_APP;
static Boolean frmSourceHandleEvent(EventPtr event) CSEC_APP;
static void RunSourceForm(FormPtr form) CSEC_APP;
static void RunSetupForm(FormPtr form) CSEC_APP;
static void output_scrollbar_update(UInt16 formID, UInt16 scrollID, UInt32 maxlines, UInt32 vislines, UInt32 curline) CSEC_APP;
static void output_scroll_lines(Int16 numlines, Boolean redraw) CSEC_APP;
static void output_scroll_page(WinDirectionType dir) CSEC_APP;
static void RunRuntimeForm(FormPtr form) CSEC_APP;


/* global vars */
#define CREATOR_CODE 'LUAP'
#define CREATOR_TEXT "LUAP"

static lua_State *L = NULL;

/* preferences */

enum output_markup_kind {
	OMK_NONE,
	OMK_MONO,
	OMK_COLOR,
};

static struct palmapp_prefs {
	UInt16	version;
	/* version 0 up to here */
	struct {
		enum output_markup_kind markup;
		int maxlines;
		int maxbytes;
		Boolean usehires;
	} output;
	struct {
		Boolean capa;
		Boolean dummy; /* enforce 2 byte alignment for the moment */
	} diag;
	/* version 1 up to here */
} preferences;
/* for the defaults see the StartApplication() routine */

/*
 * XXX TODO
 * this approach to determine sizes does not work,
 * since the total of all fields is not necessarily
 * the total size of the complete struct (alignment
 * and padding may apply)
 */

#define PREF_VERSION_SIZE0	(sizeof(UInt16))
#define PREF_VERSION_SIZE1	(PREF_VERSION_SIZE0 \
				+ sizeof(preferences.output) \
				+ sizeof(preferences.diag) \
				)
#define PREF_VERSION_SIZE	PREF_VERSION_SIZE1
#define PREF_VERSION_LATEST 1


static struct palmapp_props {
	Boolean hasColor;
	Boolean hasHires;
	Coord hiresMaxX;
	Coord hiresMaxY;
	FontID fntStdin;
	FontID fntStdout;
	FontID fntStderr;
	UInt16 fontHeight;
	IndexedColorType clrStdin;
	IndexedColorType clrStdout;
	IndexedColorType clrStderr;
	UInt16 coordSystemSave;
	UInt16 coordSystemFound;
} properties;

/* startup / cleanup code */

static Err
StartApplication(void) {
	Err err;
	UInt16 LoadPrefSize;
	UInt16 u16;
	UInt32 u32;
	char txtnum1[20], txtnum2[20];
	FontID saveFont;
	UInt16 height;
	PointType pt;

	/* check memory layout assumptions before working with the data*/
	if (output_verify_layout() != 0) {
		FrmCustomAlert(ID_FORM_FAILURE, "unexpected memory layout, aborting", "", "");
		return(sysErrNotInitialized);
	}

	/* zero out, load and optionally complete (preset) prefs */
	memset(&preferences, 0, sizeof(preferences));
	LoadPrefSize = sizeof(preferences);
	if (PrefGetAppPreferences(
		  CREATOR_CODE, 1000
		, &preferences, &LoadPrefSize
		, true) == noPreferenceFound)
		LoadPrefSize = 0;
	/* fill up new "versions" if needed */
	if ((preferences.version < 1) || (LoadPrefSize < PREF_VERSION_SIZE1)) {
		preferences.version = 1;
		preferences.output.markup = OMK_COLOR;
		preferences.output.maxlines = 50;
		preferences.output.maxbytes = 8000;
		preferences.output.usehires = false;
		preferences.diag.capa = false;
		LoadPrefSize = PREF_VERSION_SIZE1;
	}
	/* sanity check (actually: paranoia) */
	if (preferences.version != PREF_VERSION_LATEST) {
		FrmCustomAlert(ID_FORM_FAILURE, "programming error", "preferences with wrong version", "");
		preferences.version = PREF_VERSION_LATEST;
	}
	if (LoadPrefSize != PREF_VERSION_SIZE) {
		FrmCustomAlert(ID_FORM_FAILURE, "programming error", "preferences of wrong size", "");
		/* how to handle that condition? */
	}


	/* determine capabilities */
	FtrGet(sysFtrCreator, sysFtrNumProcessorID, &u32);
	if (sysFtrNumProcessorIsARM(u32)) {
		/* XXX could enable something here later */
		if (preferences.diag.capa)
			FrmCustomAlert(ID_FORM_INFORMATION, "ARM processor found (but not used)", "", "");
	}

	FtrGet(sysFtrCreator, sysFtrNumDisplayDepth, &u32);
	StrIToH(txtnum1, u32);
	if (preferences.diag.capa) {
		FrmCustomAlert(ID_FORM_INFORMATION, "display depth", txtnum1, (u32 > 1) ? "-> color" : "-> mono");
	}
	properties.hasColor = (u32 > 1) ? true : false;

	FtrGet(sysFtrCreator, sysFtrDefaultFont, &u32);
	StrIToH(txtnum1, u32);
	properties.fntStdin = u32;
	properties.fntStdout = u32;
	FtrGet(sysFtrCreator, sysFtrDefaultBoldFont, &u32);
	StrIToH(txtnum2, u32);
	properties.fntStderr = u32;
	if (preferences.diag.capa) {
		FrmCustomAlert(ID_FORM_INFORMATION, "default/bold font", txtnum1, txtnum2);
	}

	/*
	 * XXX TODO
	 * check more system features?
	 * sysFtrNumCharEncodingFlags, sysFtrNumEncoding, sysFtrNumLanguage
	 * sysFtrNumWinVersion (4.0)
	 *
	 * SysGetOrientation()
	 * hires display? which Coord values?
	 */

	properties.fontHeight = 0;
	saveFont = FntGetFont();
	FntSetFont(properties.fntStdin);
	height = FntLineHeight();
	height = FntCharHeight();
	if (properties.fontHeight < height)
		properties.fontHeight = height;
	FntSetFont(properties.fntStderr);
	height = FntLineHeight();
	if (properties.fontHeight < height)
		properties.fontHeight = height;
	FntSetFont(saveFont);
	/* XXX hack, auto detection always seems to pick "15" and
	 * then we only can display 7 lines, while 10 lines with
	 * 11 pixels each would work fine; make this a preference?
	 */
	properties.fontHeight = 11;


	/* determine current and maximum coordinate system */
	properties.hasHires = false;
	properties.hiresMaxX = 160;
	properties.hiresMaxY = 160;
	FtrGet(sysFtrCreator, sysFtrNumROMVersion, &u32);
	if (! preferences.output.usehires) {
		/* no need to detect resolution */
		err = sysErrNotInitialized;
	} else if (u32 < sysMakeROMVersion(5, 0, 0, sysROMStageRelease, 0)) {
		/* OS prior to 5.0, probably will abort on HD calls (POSE does) */
		err = sysErrNotInitialized;
	} else {
		// properties.coordSystemSave = WinSetCoordinateSystem(kCoordinatesNative);
		properties.coordSystemSave = WinSetCoordinateSystem(kCoordinatesDouble);
		err = WinGetSupportedDensity(&u16);
		properties.coordSystemFound = WinSetCoordinateSystem(kCoordinatesStandard);
		if (preferences.diag.capa) {
			StrIToA(txtnum1, err);
			StrIToA(txtnum2, u16);
			FrmCustomAlert(ID_FORM_INFORMATION, "got density", txtnum1, txtnum2);
			StrIToA(txtnum1, properties.coordSystemSave);
			StrIToA(txtnum2, properties.coordSystemFound);
			FrmCustomAlert(ID_FORM_INFORMATION, "save/found", txtnum1, txtnum2);
		}
	}
	if ((err == 0) && (properties.coordSystemFound > kCoordinatesStandard)) {
		properties.hasHires = true;
		WinSetCoordinateSystem(properties.coordSystemFound);
		pt.x = 160;
		pt.y = 160;
		WinScalePoint(&pt, true);
		WinSetCoordinateSystem(properties.coordSystemSave);
		if (preferences.diag.capa) {
			StrIToA(txtnum1, pt.x);
			StrIToA(txtnum2, pt.y);
			FrmCustomAlert(ID_FORM_INFORMATION, "max coords", txtnum1, txtnum2);
		}
		properties.hiresMaxX = pt.x;
		properties.hiresMaxY = pt.y;
	}


	/* setup properties depending on preferences and capabilities */
	{
	RGBColorType rgb;

	rgb.r = 0; rgb.g = 0; rgb.b = 255;
	properties.clrStdin = WinRGBToIndex(&rgb);
	rgb.r = 0; rgb.g = 0; rgb.b = 0;
	properties.clrStdout = WinRGBToIndex(&rgb);
	rgb.r = 255; rgb.g = 0; rgb.b = 0;
	properties.clrStderr = WinRGBToIndex(&rgb);
	}



#if 0
	/* open the source code DB */
	if (OpenSourceDatabase() != 0) {
		FrmCustomAlert(ID_FORM_FAILURE, "could not load source code DB", "", "");
		return(sysErrLibNotFound);
	}
#endif


	/* open a Lua VM */
	L = luaL_newstate();
	if (L == NULL) {
		FrmCustomAlert(ID_FORM_FAILURE, "luaL_newstate() failed", "", "");
		return(sysErrNoFreeResource);
	}
	luaL_openlibs(L);
	luaopen_palmos(L); /* XXX TODO pcall()? */
	lua_settop(L, 0); /* clear the stack */

	/* goto the form we were at last time */
	FrmGotoForm(ID_FORM_MAIN); /* XXX saved form instead of main? */
	return(0);
}

static void
StopApplication(void) {
	/* shutdown all forms */
	FrmCloseAllForms();

	/* restore screen resolution */
	if (properties.hasHires)
		WinSetCoordinateSystem(properties.coordSystemSave);

#if 0
	/* close the source database */
	CloseSourceDatabase();
#endif

	/* close the Lua VM */
	if (L != NULL)
		lua_close(L);
	L = NULL;

	/* save prefs */
	PrefSetAppPreferences(
		  CREATOR_CODE, 1000
		, sizeof(preferences)
		, &preferences, sizeof(preferences)
		, true
		);
}

/*
 * the source code database
 */
static DmOpenRef srcDB = 0;

static Err OpenSourceDatabase(void) {
	static char *hello = "print(\"hello world\\n\")";

	UInt16 mode;
	DmOpenRef ref;
	Err err;
	UInt16 idx;
	UInt32 len;
	MemHandle rec;
	char *str;
	
	err = 0;
	mode = dmModeReadWrite;
	ref = DmOpenDatabaseByTypeCreator('DATA', CREATOR_CODE, mode);
	if (! ref) {
		err = DmGetLastErr();
		if (err == dmErrCantFind) {
			err = DmCreateDatabase(0, "SourceCode-" CREATOR_TEXT, CREATOR_CODE, 'DATA', false);
			if (err)
				return(err);
			ref = DmOpenDatabaseByTypeCreator('DATA', CREATOR_CODE, mode);
			if (! ref)
				err = DmGetLastErr();
			/* seed with some "hello world" */
			idx = dmMaxRecordIndex;
			len = strlen(hello) + 1;
			rec = DmNewRecord(ref, &idx, len);
			if (rec) {
				str = MemHandleLock(rec);
				DmStrCopy(str, 0, hello);
				MemHandleUnlock(rec);
				DmReleaseRecord(ref, 0, true);
			}
		}
	}
	if (ref)
		srcDB = ref;
	return(err);
}

static void CloseSourceDatabase(void) {
	if (! srcDB)
		return;
	DmCloseDatabase(srcDB);
	srcDB = 0;
}

static MemHandle FetchSourceDatabase(void) {
	MemHandle hdl;
	MemHandle rec;
	char *dst, *src;

	hdl = 0;
	if (OpenSourceDatabase() != 0) {
		return(hdl);
	}
	rec = DmQueryRecord(srcDB, 0);
	if (! rec) {
		CloseSourceDatabase();
		return(hdl);
	}
	hdl = MemHandleNew(MemHandleSize(rec));
	if (! hdl) {
		CloseSourceDatabase();
		return(hdl);
	}
	src = MemHandleLock(rec);
	dst = MemHandleLock(hdl);
	snprintf(dst, MemHandleSize(hdl), "%s", src);
	MemHandleUnlock(hdl);
	MemHandleUnlock(rec);
	CloseSourceDatabase();
	return(hdl);
}

static void StoreSourceDatabase(MemHandle hdl) {
	MemHandle rec;
	char *dst, *src;
	UInt32 len;

	if (! hdl)
		return;
	src = MemHandleLock(hdl);
	len = strlen(src) + 1;

	OpenSourceDatabase();
	DmResizeRecord(srcDB, 0, len);
	rec = DmGetRecord(srcDB, 0);
	if (rec) {
		dst = MemHandleLock(rec);
		DmStrCopy(dst, 0, src);
		MemHandleUnlock(rec);
		DmReleaseRecord(srcDB, 0, true);
	}
	CloseSourceDatabase();
	MemHandleUnlock(hdl);
	MemHandleFree(hdl);
}

/* "OS core" and application communication */

/*
 * output echo handling
 *
 * keep a record of all previous input and output in chunks
 *
 * each chunk has a type (UInt32) and a length (UInt32, including the chunk
 * length field), and "data" (length of the chunk minus its header, arbitrary
 * structure, padded to full UInt32)
 *
 * OCT_TERM terminates the sequence of chunks
 *
 * OCT_HEAD contains some kind of TOC and mgmt data: the total allocated size
 * and free space of this chunk list, a reference to the last data chunk and
 * the terminator for faster append, a list of references where data for
 * individual screen lines start for faster redraw, accumulated information
 * about the currently last chunk to optionally combine chunks of the same kind
 * upon append
 *
 * OCT_STDIN, OCT_STDOUT, OCT_STDERR contain a flag field (UInt32), a length
 * field (UInt32) and opaque char * data part (may in theory contain non
 * printables and specials)
 *
 * no other chunk type defined yet
 */

/* lookup routines */

struct inout_desc {
	MemHandle input;
	MemHandle output;
	MemHandle script;
};

static struct inout_desc *inout_getdata(void) {
	FormPtr form;
	UInt16 idx;
	struct inout_desc *ptr;

	form = FrmGetFormPtr(ID_FORM_MAIN);
	if (form == NULL)
		return(NULL);
	idx = FrmGetObjectIndex(form, ID_FLD_MAIN_OUTPUT);
	ptr = FrmGetGadgetData(form, idx);
	return(ptr);
}

static void *inout_getgadget(FormPtr *frm, UInt16 *index) {
	FormPtr form;
	UInt16 idx;
	void *gdgt;

	if (frm != NULL)
		*frm = NULL;
	if (index != NULL)
		*index = 0;
	gdgt = NULL;

	form = FrmGetFormPtr(ID_FORM_MAIN);
	if (form == NULL)
		return(gdgt);
	idx = FrmGetObjectIndex(form, ID_FLD_MAIN_OUTPUT);
	gdgt = FrmGetObjectPtr(form, idx);

	if (frm != NULL)
		*frm = form;
	if (index != NULL)
		*index = idx;
	return(gdgt);
}

static MemHandle output_getmemhandle(void) {
	struct inout_desc *ptr;
	ptr = inout_getdata();
	return((ptr != NULL) ? ptr->output : 0);
}

static MemHandle input_getmemhandle(void) {
	struct inout_desc *ptr;
	ptr = inout_getdata();
	return((ptr != NULL) ? ptr->input : 0);
}

static void output_refreshdisplay(void) {
	FormPtr form;
	Boolean vis;

	if (FrmGetActiveFormID() != ID_FORM_MAIN)
		return;
	form = FrmGetFormPtr(ID_FORM_MAIN);
	if (form == NULL)
		return;
	vis = FrmVisible(form);
	if (! vis)
		return;
	output_dump(1);
	return;
}

/*
 * the "input" buffer is a completely differnt beast,
 * it's just "by accident" that it looks similar to
 * the output storage
 *
 * although it uses the get/put byte array routines,
 * it's not organized in chunks and is a mere buffer
 * for data the user has input but the runtime did not
 * need or could not transfer/store (at that point in
 * time, we keep it to pass it "down" later without
 * bothering the user again)
 */

struct input_chunk_layout {
	UInt32 flags;
	UInt32 char_count;
	char char_data[1];
};

static MemHandle input_init(void) {
	MemHandle hdl;
	struct input_chunk_layout *ptr;
	int len;

	len = sizeof(*ptr);
	hdl = MemHandleNew(len);
	if (! hdl)
		return(hdl);

	ptr = MemHandleLock(hdl);
	ptr->flags = 0;
	ptr->char_count = 0;
	ptr->char_data[0] = '\0'; /* actually unnecessary */
	MemHandleUnlock(hdl);

	return(hdl);
}

static void input_done(MemHandle hdl) {
	if (! hdl)
		return;

	MemHandleFree(hdl);
	return;
}

/*
 * "high level" operations on the chunk list
 * - initialization, seeding
 * - termination, releasing memory
 * - total extending to make sure space is available
 * - extending the last data chunk
 * - appending a data chunk at the end
 * - registering line break positions for later reference
 * - rotating lines which exceed limits
 * - looking up where to start displaying when a certain lines range is wanted
 */

/*
 * BEWARE!
 *
 * earlier implementations used to declare an UInt8 array and use get/put/peek
 * accessor routines to handle the fields; today we declare a struct or union
 * and assume that the compiler will put the fields where we put them in the
 * source code and keeps the order in place
 *
 * do we feel like checking that assumption upon program start?
 */

/*
 * to _completely_ avoid this crappy approach one could draw into an off screen
 * window (WinCreateOffscreenWindow()) given that the user wants to spare the
 * memory for such a thing (not sure whether that would gather speed)
 */

/*
 * notes for hires support and other stuff
 * - Noeldner 148
 * - does WinGetDisplayExtent() return the screen's capabilities?
 * - WinPushDrawState() and WinPopDrawState() to save and restore font, color
 *   and underline mode?
 * - is WinScreenMode() of any help to us?  WinScreenGetAttribute() seems to
 *   be, as is WinGetSupportedDensity()
 * - WinGetCoordinateSystem(), WinSetCoordinateSystem(), WinScaleCoord(),
 *   WinUnscaleCoord()
 */

enum output_chunk_flags {
	OCF_LINEFEED	= (1 << 0),
	OCF_LINEWRAP	= (1 << 1),
	OCF_WASDRAWN	= (1 << 2),
	OCF_SCROLLING	= (1 << 3),
};

struct output_chunk_layout {
	struct {
		UInt32 chunk_type;
		UInt32 chunk_length;
	} header;
	union outout_data_layout {
		struct {
			UInt32 mem_total;
			UInt32 mem_free;
			UInt32 offset_lastdata;
			UInt32 offset_terminator;
			UInt32 max_bytes;
			UInt32 max_lines;
			UInt32 max_x_coord;
			UInt32 max_y_coord;
			UInt32 count_lines_available;
			UInt32 count_lines_visible;
			UInt32 number_line_scrollpos;
			UInt32 count_lines_fifoed;
			UInt32 toc_flags;
			struct output_lines_table_item {
				UInt32 offset_chunk;
				UInt32 offset_char;
				UInt32 line_flags;
			} table_lines[1];
			/* BEWARE! table at the end, more items assumed */
		} toc;
		struct {
			UInt32 text_flags;
			UInt32 char_count;
			UInt32 start_x_pos;
			UInt32 start_y_pos;
			UInt32 end_x_pos;
			UInt32 end_y_pos;
			char char_data[1];
			/* BEWARE! table at the end, more items assumed */
		} text;
		struct {
			UInt32 dummy;
		} term;
	} data;
};
static struct output_chunk_layout *ocl = NULL; /* for size calculation only */
#define OUTPUT_CHUNK_LENGTH_TOC(lines) \
	(sizeof(ocl->header) + sizeof(ocl->data.toc) + \
	lines * sizeof(ocl->data.toc.table_lines[0]))
#define OUTPUT_CHUNK_LENGTH_TEXT(chars) \
	(((sizeof(ocl->header) + sizeof(ocl->data.text) + \
	chars + sizeof(UInt32) - 1) \
	/ sizeof(UInt32)) * sizeof(UInt32))
#define OUTPUT_CHUNK_LENGTH_TERMINATOR \
	(sizeof(ocl->header) + sizeof(ocl->data.term))

/* verify that assumptions about the structure's layout really do apply */
static int output_verify_layout(void) {
	/* XXX eliminate it for the moment */
	return(0);

	/* XXX TODO use assert() here */

	if (sizeof(ocl->header) != 8)
		return(-1);
	if (OffsetOf(typeof(*ocl), header.chunk_type) != 0)
		return(-1);
	if (OffsetOf(typeof(*ocl), header.chunk_length) != 4)
		return(-1);
	if (sizeof(*ocl) != sizeof(ocl->header) + sizeof(ocl->data))
		return(-1);

	if (OffsetOf(typeof(*ocl), data) != 8)
		return(-1);
	if (sizeof(ocl->data) % sizeof(UInt32) != 0)
		return(-1);

	if (OffsetOf(typeof(*ocl), data.toc) != 8)
		return(-1);
	if (sizeof(ocl->data.toc) % sizeof(UInt32) != 0)
		return(-1);
	if (OffsetOf(typeof(*ocl), data.toc.table_lines) != 56)
		return(-1);
	if (sizeof(ocl->data.toc.table_lines[0]) != 12)
		return(-1);
	if (OUTPUT_CHUNK_LENGTH_TOC(0) != sizeof(ocl->header) + sizeof(ocl->data.toc))
		return(-1);
	if (OUTPUT_CHUNK_LENGTH_TOC(1) != sizeof(ocl->header) + sizeof(ocl->data.toc) + sizeof(ocl->data.toc.table_lines[0]))
		return(-1);

	if (sizeof(ocl->data.text) % sizeof(UInt32) != 0)
		return(-1);
	if (OUTPUT_CHUNK_LENGTH_TEXT(0) != sizeof(ocl->data.text))
		return(-1);
	if (OUTPUT_CHUNK_LENGTH_TEXT(3) != sizeof(ocl->data.text))
		return(-1);
	if (OUTPUT_CHUNK_LENGTH_TEXT(4) != sizeof(ocl->data.text) + sizeof(UInt32))
		return(-1);
	if (OffsetOf(typeof(*ocl), data.text.char_data) != 32)
		return(-1);

	if (OffsetOf(typeof(*ocl), data.term) != 8)
		return(-1);
	if (sizeof(ocl->data.term) != 4)
		return(-1);

	return(0);
}

/* the unit in which we resize the chunk list */
#define OUTPUT_CHUNK_LIST_CHUNKSIZE 256

/* allocate and preset the output chunk list memory */
static MemHandle output_init(void) {
	MemHandle hdl;
	UInt8 *listptr;
	struct output_chunk_layout *tocptr, *termptr;
	UInt32 toclen, termlen, needlen, alloclen;
	FormPtr form;
	UInt16 idx;
	RectangleType bounds;
	Coord xmax, ymax;

	/* determine the size of the TOC chunk */
	toclen = OUTPUT_CHUNK_LENGTH_TOC(preferences.output.maxlines);
	termlen = OUTPUT_CHUNK_LENGTH_TERMINATOR;

	/* determine the required memory size */
	needlen = toclen + termlen;

	/* determine the initial memory allocation size */
	alloclen = needlen;
	alloclen += OUTPUT_CHUNK_LIST_CHUNKSIZE - 1;
	alloclen /= OUTPUT_CHUNK_LIST_CHUNKSIZE;
	alloclen *= OUTPUT_CHUNK_LIST_CHUNKSIZE;

	/* get the memory block */
	hdl = MemHandleNew(alloclen);
	if (! hdl)
		return(hdl);
	listptr = MemHandleLock(hdl);
	memset(listptr, 0, alloclen);

	/* write out initial data ... */
	tocptr = (struct output_chunk_layout *)listptr;
	listptr += toclen;
	termptr = (struct output_chunk_layout *)listptr;
	listptr += termlen;


	/* the chunk list head's chunk header */
	tocptr->header.chunk_type = OCT_HEAD;
	tocptr->header.chunk_length = toclen;

	/* memory allocation data */
	tocptr->data.toc.mem_total = alloclen;
	tocptr->data.toc.mem_free = alloclen - needlen;

	/* offset of last data chunk and terminator chunk */
	tocptr->data.toc.offset_lastdata = 0;
	tocptr->data.toc.offset_terminator = toclen;

	/* output maximum values */
	tocptr->data.toc.max_bytes = preferences.output.maxbytes;
	tocptr->data.toc.max_lines = preferences.output.maxlines;

	/* we need the output window's extent */
	if (inout_getgadget(&form, &idx) != NULL) {
		FrmGetObjectBounds(form, idx, &bounds);
		xmax = bounds.extent.x;
		ymax = bounds.extent.y;
	} else {
		/* ShouldNeverHappen(TM) */
		xmax = 160;
		ymax = 160;
	}
	tocptr->data.toc.max_x_coord = xmax;
	tocptr->data.toc.max_y_coord = ymax;

	/* avail/visible/scroll line numbers */
	tocptr->data.toc.toc_flags = 0;
	tocptr->data.toc.count_lines_available = 1;
	tocptr->data.toc.count_lines_visible = ymax / properties.fontHeight;
	tocptr->data.toc.number_line_scrollpos = 1;
	output_scrollbar_update(ID_FORM_MAIN, ID_SCR_MAIN_OUTPUT
				, tocptr->data.toc.count_lines_available
				, tocptr->data.toc.count_lines_visible
				, tocptr->data.toc.number_line_scrollpos
				);

	/* screen line positions (YES, the array size is maxlines + 1) */
	for (idx = 0; idx <= preferences.output.maxlines; idx++) {
		tocptr->data.toc.table_lines[idx].offset_chunk = 0;
		tocptr->data.toc.table_lines[idx].offset_char = 0;
		tocptr->data.toc.table_lines[idx].line_flags = 0;
	}
	tocptr->data.toc.table_lines[0].offset_chunk = tocptr->data.toc.offset_terminator;
	tocptr->data.toc.table_lines[0].offset_char = 0;


	/* chunk list terminator (terminating chunk's header) */
	termptr->header.chunk_type = OCT_TERM;
	termptr->header.chunk_length = termlen;
	termptr->data.term.dummy = 0xb000b000L;


	MemHandleUnlock(hdl);
	return(hdl);
}

/* release the output chunk list memory */
static void output_done(MemHandle hdl) {
	if (! hdl)
		return;

	MemHandleFree(hdl);
	return;
}

/* completely clear the output chunk list (just free and realloc) */
static void output_clear(void) {
	struct inout_desc *desc;

	desc = inout_getdata();
	if (desc == NULL)
		return;

	output_done(desc->output);
	desc->output = output_init();
	return;
}

/* resize the output chunk list, "minadd" bytes will be needed soon */
static int output_resize(MemHandle hdl, const UInt32 minadd) {
	UInt8 *listptr;
	struct output_chunk_layout *chunk;
	UInt32 alloclen, freesize, addlen;
	int ok;
	int bResize;

	listptr = MemHandleLock(hdl);
	chunk = (struct output_chunk_layout *)listptr;

	ok = 1;

	/* test chunk type, skip chunk length */
	if (ok) {
		if (chunk->header.chunk_type != OCT_HEAD) ok = 0;
	}

	/* get total alloc size and free space */
	if (ok) {
		alloclen = chunk->data.toc.mem_total;
		freesize = chunk->data.toc.mem_free;
	}

	/* need more room than is available? */
	bResize = (minadd >= freesize) ? 1 : 0;
	if (ok && bResize) {
		/* add "min add" to the current size, round up */
		addlen = minadd;
		addlen += OUTPUT_CHUNK_LIST_CHUNKSIZE - 1;
		addlen /= OUTPUT_CHUNK_LIST_CHUNKSIZE;
		addlen *= OUTPUT_CHUNK_LIST_CHUNKSIZE;

		/* resize the memory block */
		MemHandleUnlock(hdl);
		if (MemHandleResize(hdl, alloclen + addlen) != 0) ok = 0;
		listptr = MemHandleLock(hdl);
	}

	/* update header data */
	if (ok && bResize) {
		chunk = (struct output_chunk_layout *)listptr;
		chunk->data.toc.mem_total = alloclen + addlen;
		chunk->data.toc.mem_free = freesize + addlen;
	}

	MemHandleUnlock(hdl);
	return((ok) ? 0 : -1);
}

/* setup font/markup depending on preferences.output.markup */
static void output_markup_prepare(const enum output_chunk_type t, FontID *fntstore, IndexedColorType *clrstore, UnderlineModeType *ulstore) {
	IndexedColorType clr, clrprev;
	FontID fnt, fntprev;
	UnderlineModeType ul, ulprev;

	if ((preferences.output.markup == OMK_COLOR) && (properties.hasColor)) {
		/* color wanted and available */
		switch (t) {
		case OCT_STDIN:
			clr = properties.clrStdin;
			break;
		case OCT_STDOUT:
			clr = properties.clrStdout;
			break;
		case OCT_STDERR:
			clr = properties.clrStderr;
			break;
		default:
			clr = properties.clrStdout;
			break;
		}
		clrprev = WinSetTextColor(clr);
		if (clrstore != NULL)
			*clrstore = clrprev;
	} else if (preferences.output.markup != OMK_NONE) {
		/* color wanted and not available, or mono wanted */
		switch (t) {
		case OCT_STDIN:
			ul = solidUnderline;
			fnt = stdFont;
			break;
		case OCT_STDOUT:
			ul = noUnderline;
			fnt = stdFont;
			break;
		case OCT_STDERR:
			ul = noUnderline;
			fnt = boldFont;
			break;
		default:
			ul = noUnderline;
			fnt = stdFont;
			break;
		}
		ulprev = WinSetUnderlineMode(ul);
		fntprev = FntSetFont(fnt);
		if (ulstore != NULL)
			*ulstore = ulprev;
		if (fntstore != NULL)
			*fntstore = ulprev;
	} else {
		/* no markup wanted */
		/* EMPTY */
	}
}

/* restore previous font settings */
static void output_markup_restore(FontID fntprev, IndexedColorType clrprev, UnderlineModeType ulprev) {
	if ((preferences.output.markup == OMK_COLOR) && (properties.hasColor)) {
		WinSetTextColor(clrprev);
	} else if (preferences.output.markup != OMK_NONE) {
		WinSetUnderlineMode(ulprev);
		FntSetFont(fntprev);
	} else {
		/* EMPTY */
	}
}

/*
 * XXX TODO
 * keep track of the number of lines and the lines' width,
 * rotate the text in a FIFO fashion to not exceed a given size,
 * update the scrollbar(s) (see ORA pp 277)
 */

/* append a new chunk or extend the last one if possible */
static int output_addchunk(MemHandle hdl, enum output_chunk_type type, UInt8 *data, UInt32 size) {
	UInt8 *listptr;
	struct output_chunk_layout *tocptr, *lastchunk, *newchunk;
	UInt32 lastoffset, termoffset, newoffset, distance;
	int neednew;
	UInt32 flags;
	Coord x, y;
	Coord xmax, ymax;
	char *txtptr;
	int txtlen;
	IndexedColorType clr;
	FontID fnt;
	UnderlineModeType ul;
	UInt32 refoffset, reloffset;
	struct output_chunk_layout *refchunk, *relchunk;
	int idx;

	if (! hdl)
		return(-1);
	if ((type != OCT_STDIN) && (type != OCT_STDOUT) && (type != OCT_STDERR))
		return(-1);
	if (data == NULL)
		return(-1);
	if (size <= 0)
		return(-1);

	/* make sure enough space is available */
	if (output_resize(hdl, size + OUTPUT_CHUNK_LENGTH_TEXT(size)) != 0)
		return(-1);

	/* linefeed is a flag of the chunk, we don't store the LF character */
	flags = 0;
	if ((size > 0) && (data[size - 1] == '\n')) {
		flags |= OCF_LINEFEED;
		size--;
	}

	/* lock the memory */
	listptr = MemHandleLock(hdl);

	/* locate the last data chunk */
	tocptr = (struct output_chunk_layout *)listptr;
	if (tocptr->header.chunk_type != OCT_HEAD) return(-1);
	lastoffset = tocptr->data.toc.offset_lastdata;
	lastchunk = (struct output_chunk_layout *)(listptr + lastoffset);
	switch (lastchunk->header.chunk_type) {
	case OCT_STDIN:
	case OCT_STDOUT:
	case OCT_STDERR:
		/* EMPTY */
		break;
	default:
		lastchunk = NULL;
		break;
	}
	termoffset = tocptr->data.toc.offset_terminator;

	/* check whether we can reuse (extend) the last chunk */
	neednew = 0;
	if (lastchunk == NULL) {
		/* no data chunks yet, need a new one */
		neednew = 1;
	} else {
		/* of different type, need a new one */
		if (lastchunk->header.chunk_type != type)
			neednew = 1;
		/* result exceeds an arbitrary size, need a new one */
		if (lastchunk->header.chunk_length + size > 128)
			neednew = 1;
		/* already ends with a newline, need a new one */
		if (lastchunk->data.text.text_flags & OCF_LINEFEED)
			neednew = 1;
		/* caused a line wrap, need a new one */
		if (lastchunk->data.text.text_flags & OCF_LINEWRAP)
			neednew = 1;
	}

	/* XXX we may have to adjust the text data reference for the last
	 * screen line should the last drawn chunk have had the line feed or
	 * break exactly at its char_data end */

	if (neednew) {
		/* insert new chunk header and space for payload */

		/* determine needed space for the new chunk */
		distance = OUTPUT_CHUNK_LENGTH_TEXT(size);

		/* move the terminator towards the end */
		newoffset = termoffset;
		termoffset += distance;
		memmove(listptr + termoffset, listptr + newoffset, OUTPUT_CHUNK_LENGTH_TERMINATOR);
		memset(listptr + newoffset, 0, distance);

		/* update the free size field */
		tocptr->data.toc.mem_free -= distance;

		/* update the last data and terminator chunk offsets */
		tocptr->data.toc.offset_lastdata = newoffset;
		tocptr->data.toc.offset_terminator = termoffset;

		/* write the chunk header for the new data block */
		newchunk = (struct output_chunk_layout *)(listptr + newoffset);
		newchunk->header.chunk_type = type;
		newchunk->header.chunk_length = distance;

		/* write the "in chunk" "header" fields and learn where to go on drawing */
		newchunk->data.text.text_flags = flags;
		newchunk->data.text.char_count = size;
		if (lastoffset == 0) {
			x = newchunk->data.text.start_x_pos = 0;
			y = newchunk->data.text.start_y_pos = 0;
		} else {
			x = newchunk->data.text.start_x_pos = lastchunk->data.text.end_x_pos;
			y = newchunk->data.text.start_y_pos = lastchunk->data.text.end_y_pos;
		}
		txtptr = newchunk->data.text.char_data;
		txtlen = size;
		memmove(txtptr, data, size);
	} else {
		/* we reuse an existing chunk, extend it */
		UInt32 oldsize, oldcount;
		UInt32 newsize, newcount;

		newoffset = lastoffset;
		newchunk = (struct output_chunk_layout *)(listptr + newoffset);

		/* add new char length to the old one, determine new chunk size */
		oldcount = newchunk->data.text.char_count;
		oldsize = newchunk->header.chunk_length;
		newcount = oldcount + size;
		newsize = OUTPUT_CHUNK_LENGTH_TEXT(newcount);

		/* move the terminator towards the end */
		distance = newsize - oldsize;
		memmove(listptr + termoffset + distance, listptr + termoffset, OUTPUT_CHUNK_LENGTH_TERMINATOR);
		memset(listptr + termoffset, 0, distance);
		termoffset += distance;

		/* update the free size field */
		tocptr->data.toc.mem_free -= distance;

		/* update the terminator chunk offset */
		tocptr->data.toc.offset_terminator = termoffset;

		/* update the text chunk's header */
		newchunk->header.chunk_length = newsize;

		/* merge our text flags, reset "drawn" status */
		flags |= newchunk->data.text.text_flags;
		flags &= ~OCF_WASDRAWN;
		newchunk->data.text.text_flags = flags;

		/* write the "in chunk" "header" fields and learn where to go on drawing */
		newchunk->data.text.char_count = newcount;
		x = newchunk->data.text.end_x_pos;
		y = newchunk->data.text.end_y_pos;
		txtptr = newchunk->data.text.char_data + oldcount;
		txtlen = size;
		memmove(txtptr, data, size);
	}

	/* we need the output window's extent */
	xmax = tocptr->data.toc.max_x_coord;
	ymax = tocptr->data.toc.max_y_coord;
	if (properties.hasHires) {
		WinSetCoordinateSystem(properties.coordSystemFound);
		xmax = WinScaleCoord(xmax, true);
		ymax = WinScaleCoord(ymax, true);
		WinSetCoordinateSystem(properties.coordSystemSave);
	}

	/*
	 * format the text (don't draw the characters on screen
	 * but determine their coords and line breaks)
	 */

	/*
	 * setup screen to reflect the type of text data
	 * according to the user's preferences
	 */
	if (properties.hasHires) {
		WinSetCoordinateSystem(properties.coordSystemFound);
	}
	output_markup_prepare(type, &fnt, &clr, &ul);

	/* usually just one iteration, more only upon line wraps */
	do {

		int wr, reason;
		UInt16 w; /* is this the same as Coord? FntWidth*() needs UInt16 ... */
		struct output_lines_table_item *tab;

		tab = tocptr->data.toc.table_lines;

		/*
		 * scroll up the text coordinates if needed
		 *
		 * YES, I consider this a feature that scrolling
		 * only takes place as a yet empty line finally
		 * has some data on it to actually print it,
		 * screen space is so precious on a Palm device :)
		 */
		if (y + properties.fontHeight >= ymax) {

			/*
			 * update ("scroll") all previous text chunks,
			 * decrement all "y" positions by the font height
			 */
			lastchunk = tocptr;
			if (0) do {
				if (lastchunk == newchunk)
					break;

				/* should never happen */
				if (lastchunk->header.chunk_type == OCT_TERM)
					break;

				switch (lastchunk->header.chunk_type) {
				case OCT_STDIN:
				case OCT_STDOUT:
				case OCT_STDERR:
					lastchunk->data.text.start_y_pos -= properties.fontHeight;
					lastchunk->data.text.end_y_pos -= properties.fontHeight;
					break;
				default:
					/* EMPTY */
					break;
				}

				lastchunk = (struct output_chunk_layout *)(((UInt8 *)lastchunk) + lastchunk->header.chunk_length);
			} while (1);

			/*
			 * continue positioning characters one line above
			 * our currently reached position
			 */
			y -= properties.fontHeight;

		}

		/* determine which part of the text would fit into the window width */
		wr = FntWidthToOffset(txtptr, txtlen, xmax - x, NULL, &w);

		/* skip over that part (may be empty) and advance the cursor */
		if (wr > 0) {
			txtptr += wr;
			txtlen -= wr;
			x += w;
		}

		/* keep track of where new lines started */
		reason = 0;
		if (txtlen != 0) {
			/* mere line wrap (check first!) */
			reason = OCF_LINEWRAP;
		} else if (newchunk->data.text.text_flags & OCF_LINEFEED) {
			/* real line break */
			reason = OCF_LINEFEED;
		}
		if (reason != 0) {	/* line broken or wrapped? */

			/* position cursor on new line's beginning */
			y += properties.fontHeight;
			x = 0;

			/*
			 * mark this chunk and its screen line as "line broken"
			 * or "line wrapped" (prevents its getting extended / merged
			 * with later added data, which would either falsify our
			 * draw status or would be next to impossible to handle)
			 * XXX update: does that still bother us?
			 */
			newchunk->data.text.text_flags |= reason;
			tab[tocptr->data.toc.count_lines_available - 1].line_flags |= reason;

			/*
			 * store the position of the text data for the
			 * newly started screen line (either continued here
			 * with real data or pointing to the very end of
			 * the char_data -- when this chunks gets extended
			 * the reference is correct, if not the draw routine
			 * just needs to skip this "empty remainder" which
			 * is a rather simple task)
			 *
			 * increment the "availalbe lines" counter
			 */
			tab[tocptr->data.toc.count_lines_available].offset_chunk = newoffset;
			tab[tocptr->data.toc.count_lines_available].offset_char = txtptr - newchunk->data.text.char_data;
			tab[tocptr->data.toc.count_lines_available].line_flags = 0;
			tocptr->data.toc.count_lines_available++;

			/*
			 * shift the lines table upon overflow and
			 * clear the new last item
			 */
			if (tocptr->data.toc.count_lines_available > tocptr->data.toc.max_lines) {
				memmove(&tab[0], &tab[1], sizeof(tab[0]) * (tocptr->data.toc.max_lines));
				tab[tocptr->data.toc.count_lines_available].offset_chunk = 0;
				tab[tocptr->data.toc.count_lines_available].offset_char = 0;
				tab[tocptr->data.toc.count_lines_available].line_flags = 0;

				tocptr->data.toc.count_lines_fifoed++;
				tocptr->data.toc.count_lines_available--;
			}

			/*
			 * if currently not in scrolling mode, act like
			 * the user had "scrolled down" to see the new
			 * end of the screen output
			 *
			 * update the scollbar
			 */
			if (! (tocptr->data.toc.toc_flags & OCF_SCROLLING)) {
				if (tocptr->data.toc.number_line_scrollpos + tocptr->data.toc.count_lines_visible <= tocptr->data.toc.count_lines_available)
					tocptr->data.toc.number_line_scrollpos++;
			}
			output_scrollbar_update(ID_FORM_MAIN, ID_SCR_MAIN_OUTPUT
					, tocptr->data.toc.count_lines_available
					, tocptr->data.toc.count_lines_visible
					, tocptr->data.toc.number_line_scrollpos
					);
		}

	} while (txtlen > 0);

	/* store pixel coord we just reached and mark chunk as drawn when complete */
	newchunk->data.text.text_flags |= OCF_WASDRAWN;
	newchunk->data.text.end_x_pos = x;
	newchunk->data.text.end_y_pos = y;

	/* restore screen settings */
	output_markup_restore(fnt, clr, ul);
	if (properties.hasHires) {
		WinSetCoordinateSystem(properties.coordSystemSave);
	}

	/* release excess chunks which are off display and history */
	/* determine what's still referenced and what could be released */
	refoffset = tocptr->data.toc.table_lines[0].offset_chunk;
	reloffset = tocptr->header.chunk_length;
	if (refoffset > reloffset) {
		/* we have unreachable chunks (which fell of the screen lines history) */
		refchunk = (struct output_chunk_layout *)(listptr + refoffset);
		relchunk = (struct output_chunk_layout *)(listptr + reloffset);

		/* move still referenced chunks to the front, update "free" TOC field */
		distance = refoffset - reloffset;
		memmove(relchunk, refchunk, tocptr->data.toc.mem_total - refoffset);
		tocptr->data.toc.mem_free += distance;

		/* update all chunk offset references in the TOC */
		tocptr->data.toc.offset_lastdata -= distance;
		tocptr->data.toc.offset_terminator -= distance;
		for (idx = 0; idx <= tocptr->data.toc.max_lines; idx++) {
			if (tocptr->data.toc.table_lines[idx].offset_chunk)
				tocptr->data.toc.table_lines[idx].offset_chunk -= distance;
		}

		/* chunks have their individual length but don't reference each other */
	}

	/* release the output chunk list and be done */
	MemHandleUnlock(hdl);
	return(0);
}

/* inspired by O'Reilly pp 279 */
static void output_scrollbar_update(UInt16 formID, UInt16 scrollID, UInt32 maxlines, UInt32 vislines, UInt32 curline) {
	FormPtr form;
	UInt16 idx;
	ScrollBarPtr scroll;
	Int16 maxvalue;
	Int16 minvalue;

	form = FrmGetFormPtr(formID);
	if (form == NULL)
		return;
	idx = FrmGetObjectIndex(form, scrollID);
	if (! idx)
		return;
	scroll = FrmGetObjectPtr(form, idx);
	if (scroll == NULL)
		return;

	if (maxlines > vislines) {
		/*
		 * more lines available than can be displayed,
		 * set max so that any of the previous lines
		 * could be selected to be the top display line
		 */
		minvalue = 1;
		maxvalue = maxlines - vislines + 1;
	} else {
		/*
		 * all available lines can be displayed,
		 * set max to the last line (which already
		 * gets displayed, but that doesn't matter)
		 */
		minvalue = 1;
		maxvalue = 1;
	}

	SclSetScrollBar(scroll, curline, 1, maxvalue, vislines - 1);
}

static void output_scroll_lines(Int16 numlines, Boolean redraw) {
	MemHandle hdl;
	struct output_chunk_layout *ptr;

	if (numlines == 0)
		return;

	hdl = output_getmemhandle();
	if (! hdl)
		return;
	ptr = MemHandleLock(hdl);

	if (numlines < 0) {
		/* scrolling backwards */
		numlines = -numlines;
		if (numlines >= ptr->data.toc.number_line_scrollpos)
			ptr->data.toc.number_line_scrollpos = 1;
		else
			ptr->data.toc.number_line_scrollpos -= numlines;
		/*
		 * only raise "scrolling" flag and suppress further drawing
		 * when more available than visible? or consider this a feature
		 * that "scroll up" with not yet filled up screen pauses output,
		 * too?
		 */
		ptr->data.toc.toc_flags |= OCF_SCROLLING;
	} else {
		/* scrolling forwards */
		ptr->data.toc.number_line_scrollpos += numlines;
		/* but not beyond "available" */
		if (ptr->data.toc.number_line_scrollpos > ptr->data.toc.count_lines_available)
			ptr->data.toc.number_line_scrollpos = ptr->data.toc.count_lines_available;
		/* output's end completely visible again? */
		if (ptr->data.toc.number_line_scrollpos + ptr->data.toc.count_lines_visible > ptr->data.toc.count_lines_available) {
			if (ptr->data.toc.count_lines_visible > ptr->data.toc.count_lines_available)
				ptr->data.toc.number_line_scrollpos = 1;
			else
				ptr->data.toc.number_line_scrollpos = ptr->data.toc.count_lines_available - ptr->data.toc.count_lines_visible + 1;
			ptr->data.toc.toc_flags &= ~OCF_SCROLLING;
		}
	}

	if (redraw) {
		output_scrollbar_update(ID_FORM_MAIN, ID_SCR_MAIN_OUTPUT
				, ptr->data.toc.count_lines_available
				, ptr->data.toc.count_lines_visible
				, ptr->data.toc.number_line_scrollpos
				);
	}

	MemHandleUnlock(hdl);

	output_dump(1);
}

static void output_scroll_page(WinDirectionType dir) {
	MemHandle hdl;
	struct output_chunk_layout *ptr;
	Int16 numlines;

	if ((dir != winUp) && (dir != winDown))
		return;

	hdl = output_getmemhandle();
	if (! hdl)
		return;
	ptr = MemHandleLock(hdl);

	if (dir == winUp) {
		numlines = ptr->data.toc.count_lines_visible - 1;
		numlines = -numlines;
	} else if (dir == winDown) {
		numlines = ptr->data.toc.count_lines_visible - 1;
	} else {
		numlines = 0;
	}

	MemHandleUnlock(hdl);

	output_scroll_lines(numlines, true);
	return;
}

/* draw (paint) stored output to screen */
static int output_dump(const int optimize) {
	static int last_scroll_pos;
	static int last_avail_count;

	Coord xtopleft, ytopleft;
	MemHandle hdl;
	UInt8 *chunk;
	struct output_chunk_layout *tocptr, *datptr;
	int startline, endline, line;
	FormPtr form;
	UInt16 idx;
	RectangleType bounds;
	Coord x, y;
	UInt32 startchunk, startchar;
	UInt32 endchunk, endchar;
	UInt32 txtlen;
	const char *txtptr;

	/* we'll need the gadget's data below */
	if (inout_getgadget(&form, &idx) == NULL)
		return(-1);

	/* get a reference to the chunk list */
	hdl = output_getmemhandle();
	if (! hdl)
		return(-1);
	chunk = MemHandleLock(hdl);

	/* lookup which lines we have to draw */
	tocptr = (struct output_chunk_layout *)chunk;
	if (tocptr->header.chunk_type != OCT_HEAD) {
		MemHandleUnlock(hdl);
		return(-1);
	}
	startline = tocptr->data.toc.number_line_scrollpos;
	if (startline < 1)
		startline = 1;
	endline = startline + tocptr->data.toc.count_lines_visible - 1;
	if (endline > tocptr->data.toc.count_lines_available)
		endline = tocptr->data.toc.count_lines_available;

	/* get the output window's topleft corrdinates, map normalized text corrds towards inside the window */
	FrmGetObjectBounds(form, idx, &bounds);
	xtopleft = bounds.topLeft.x;
	ytopleft = bounds.topLeft.y;
	y = 0;

	/*
	 * catch up with the screen lines ring buffer having dropped
	 * lines as they exceeded the max number of lines to manage
	 */
	if (tocptr->data.toc.count_lines_fifoed > 0) {
		RectangleType vacancy;

		if (tocptr->data.toc.count_lines_fifoed >= tocptr->data.toc.count_lines_visible) {
			WinEraseRectangle(&bounds, 0);
		} else {
			memset(&vacancy, 0, sizeof(vacancy));
			WinScrollRectangle(&bounds, winUp, tocptr->data.toc.count_lines_fifoed * properties.fontHeight, &vacancy);
			WinEraseRectangle(&vacancy, 0);
		}

		tocptr->data.toc.count_lines_fifoed = 0;
	}

	/*
	 * optionally reuse part of the previous output (scroll window up/down)
	 * and reduce what needs drawing now (only draw previously unseen part)
	 */
	if (1 && optimize) do {
		int distance, keepheight;
		WinDirectionType direction;
		RectangleType vacancy;

		memset(&vacancy, 0, sizeof(vacancy));
		if (tocptr->data.toc.number_line_scrollpos < last_scroll_pos) {
			/* scrolled backwards since last drawing */
			distance = last_scroll_pos - tocptr->data.toc.number_line_scrollpos;
			keepheight = tocptr->data.toc.count_lines_visible - distance;
			direction = winDown;
			if (distance >= tocptr->data.toc.count_lines_visible) {
				break;
			}
			WinScrollRectangle(&bounds, direction, distance * properties.fontHeight, &vacancy);
			bounds = vacancy;	/* gets erased below */
			endline = startline + distance;
		} else if (tocptr->data.toc.number_line_scrollpos > last_scroll_pos) {
			/* scrolled forward since last drawing */
			distance = tocptr->data.toc.number_line_scrollpos - last_scroll_pos;
			keepheight = tocptr->data.toc.count_lines_visible - distance;
			direction = winUp;
			if (distance >= tocptr->data.toc.count_lines_visible) {
				break;
			}
			WinScrollRectangle(&bounds, direction, distance * properties.fontHeight, &vacancy);
			bounds = vacancy;	/* gets erased below */
			startline += keepheight - 1;
			y += properties.fontHeight * (keepheight - 1);
		} else {
			/* either no scrolling at all, but most likely refresh after new output was added */
			bounds.extent.y = 0;	/* nothing to erase */
			keepheight = last_avail_count - tocptr->data.toc.number_line_scrollpos - 1;
			startline += keepheight - 1;
			y += properties.fontHeight * (keepheight - 1);
			if (startline < 1) {
				startline = 1;
				y = 0;
			}
		}

	} while (0);
	last_scroll_pos = tocptr->data.toc.number_line_scrollpos;
	last_avail_count = tocptr->data.toc.count_lines_available;

	WinEraseRectangle(&bounds, 0);

	/* save screen setup, gets changed as text is drawn */
	WinPushDrawState();
	if (properties.hasHires) {
		WinSetCoordinateSystem(properties.coordSystemFound);
	}

	/* draw the visible lines of the saved output */
	for (line = startline; line <= endline; line++) {

		/* see where text data for the line starts end ends */
		startchunk = tocptr->data.toc.table_lines[line - 1].offset_chunk;
		startchar = tocptr->data.toc.table_lines[line - 1].offset_char;
		endchunk = tocptr->data.toc.table_lines[line].offset_chunk;
		endchar = tocptr->data.toc.table_lines[line].offset_char;

		/* start drawing text from the left edge */
		x = 0;

		/* keep drawing until the line is complete */
		do {

			Coord xpos, ypos;

			/* get the first text data */
			datptr = (struct output_chunk_layout *)(chunk + startchunk);
			txtptr = datptr->data.text.char_data + startchar;
			txtlen = datptr->data.text.char_count - startchar;

			/* text exhausted (we're on the last available line */
			if (datptr->header.chunk_type == OCT_TERM)
				break;
			/* should not happen, we just abort drawing */
			if ((datptr->header.chunk_type != OCT_STDIN) &&
			    (datptr->header.chunk_type != OCT_STDOUT) &&
			    (datptr->header.chunk_type != OCT_STDERR) &&
			    1)
				break;
			/* this actually can happen for the first line which is not yet "fed or broken" */
			if (0 && (startchunk > endchunk))
				break;

			/* empty "remainder" before line breaks, skip drawing */
			if (txtlen == 0) {
				startchunk += datptr->header.chunk_length;
				startchar = 0;
				continue;
			}

			output_markup_prepare(datptr->header.chunk_type, NULL, NULL, NULL);

			xpos = x + xtopleft;
			ypos = y + ytopleft;
			if (properties.hasHires) {
				xpos = WinUnscaleCoord(xpos, true);
				ypos = WinUnscaleCoord(ypos, true);
			}

			/* print the remainder of the line (it's completely inside this very chunk) */
			if (startchunk == endchunk) {
				txtlen = endchar - startchar;
				WinDrawChars(txtptr, txtlen, xpos, ypos);
				break;
			}
			/* line spread across multiple chunks, print the
			 * chunk's remainder and advance to the next chunk */
			WinDrawChars(txtptr, txtlen, xpos, ypos);
			x = datptr->data.text.end_x_pos;
			startchunk += datptr->header.chunk_length;
			startchar = 0;

		} while (1);

		/* this line is done, increment y to draw the next line below */
		y += properties.fontHeight;

	}

	/* restore screen setup */
	if (properties.hasHires) { /* done by popdrawstate() already? */
		WinSetCoordinateSystem(properties.coordSystemSave);
	}
	WinPopDrawState();

	/* release the chunk list, be done */
	MemHandleUnlock(hdl);
	return(0);
}

/* mirror input we just received, or add newly generated output */
static void app_output_show(const enum app_event_code code, union app_event_desc *data) {
	char *txt, *sep;
	int len;
	enum output_chunk_type type;
	MemHandle hdl;

	txt = NULL;
	len = 0;
	type = OCT_TERM;
	switch (code) {
	case APP_EVENT_STDIN:
		txt = data->stdio.buffer;
		len = data->stdio.read_count;
		type = OCT_STDIN;
		break;
	case APP_EVENT_STDOUT:
		txt = data->stdio.buffer;
		len = data->stdio.length;
		type = OCT_STDOUT;
		break;
	case APP_EVENT_STDERR:
		txt = data->stdio.buffer;
		len = data->stdio.length;
		type = OCT_STDERR;
		break;
	default:
		/* EMPTY */
		break;
	}

	hdl = output_getmemhandle();
	if ((type != OCT_TERM) && (hdl)) {
		/* add individual chunks for lines (eases rotation) */
		do {
			sep = strchr(txt, '\n');
			if (sep == NULL) {
				output_addchunk(hdl, type, txt, len);
				break;
			}
			sep++;

			output_addchunk(hdl, type, txt, sep - txt);
			len -= sep - txt;
			txt = sep;
		} while (1);

		/* have the screen refreshed */
		output_refreshdisplay();
	}
}

/* callback for FrmCustomResponseAlert(), accepts anything */
static Boolean app_input_callback(Int16 btn, Char *txt) {
	return(true);
}

/*
 * get the input data from an existing buffer
 * and/or request more from the user
 *
 * handle the "want lines" case, append the newline
 *
 * input buffer layout:
 * - UInt32 flags (currently EOF only)
 * - UInt32 fill index
 * - UInt8[] buffered data
 */

enum input_flags {
	INPF_ERROR	= (1 << 0),
	INPF_EOFPEND	= (1 << 1),
	INPF_EOFSENT	= (1 << 2),
};

static void app_input_get(const enum app_event_code code, union app_event_desc *data) {
	MemHandle hdl;
	struct input_chunk_layout *ptr;
	int rc;
	Boolean sufficient;
	int copied;
	UInt8 chr;

	memset(data->stdio.buffer, 0, data->stdio.length);
	data->stdio.read_count = 0;
	/* XXX reset had_error and had_eof here? */
	sufficient = false;

	/* get a handle to the buffered information */
	hdl = input_getmemhandle();
	if (! hdl) {
		data->stdio.had_error = 1;
		return;
	}
	ptr = MemHandleLock(hdl);

	/* already sent EOF to the runtime? keep doing so */
	if (ptr->flags & INPF_EOFSENT) {
		data->stdio.had_eof = 1;
		sufficient = true;
	}

	/* no buffered data available but EOF pending? send it now */
	if ((ptr->char_count == 0) && (ptr->flags & INPF_EOFPEND)) {
		ptr->flags |= INPF_EOFSENT;
		ptr->flags &= ~INPF_EOFPEND;
		data->stdio.had_eof = 1;
		sufficient = true;
	}

	/* need data, and buffered data available? */
	if ((! sufficient) && (ptr->char_count > 0)) {

		/* copy data from the buffer to the caller */
		do {
			/* the caller's buffer is full? */
			if (data->stdio.read_count == data->stdio.length) {
				sufficient = true;
				break;
			}
			/* the buffered data is exhausted? */
			if (data->stdio.read_count == ptr->char_count) {
				break;
			}
			/* get another byte from the buffer */
			chr = ptr->char_data[data->stdio.read_count];
			data->stdio.buffer[data->stdio.read_count] = chr;
			data->stdio.read_count++;
			/* caller wants full lines and we just completed one? */
			if ((data->stdio.delimiter == '\n') && (chr == '\n')) {
				sufficient = true;
				break;
			}
		} while (1);

		/* consume the copied bytes (shift buffer, update header) */
		if (data->stdio.read_count > 0) {
			/* abuse "rc" as "read count" for the moment */
			rc = data->stdio.read_count;
			memmove(ptr->char_data, ptr->char_data + rc, ptr->char_count - rc);
			ptr->char_count -= rc;
		}

	}

	/* still hungry for more? query data from the user */
	if (! sufficient) do {
		int length;
		char *buffer;

		/* allocate some space */
		/* XXX random number, at minimum runtime's length */
		length = 256;
		if (length < data->stdio.length)
			length = data->stdio.length;
		buffer = malloc(length + 2); /* optional CR, and NUL */
		if (buffer == NULL) {
			ptr->flags |= INPF_ERROR;
			break;
		}
		memset(buffer, 0, length + 2);

		/* get input from the user */
		/*
		 * XXX optionally visualize a prompt somewhere
		 * and enable an input field instead of popping
		 * up some dialog which covers previous output
		 */
		rc = FrmCustomResponseAlert(ID_FORM_STDIN
			, "please provide input"
			, (data->stdio.delimiter == '\n') ? "(want a whole line)" : ""
			, "" /* "up to ... chars" info? */
			, buffer, length + 1, app_input_callback
			);
		length = strlen(buffer);
		if (rc == 2) {
			ptr->flags |= INPF_EOFPEND;
		} else if (rc == 1) {
			buffer[length] = '\n';
			length++;
		}

		/* fill some more into the caller's buffer */
		copied = 0;
		do {
			/* the buffered data is exhausted? */
			/* catches "length == 0" aka empty input, too */
			if (copied == length) {
				break;
			}
			/* the caller's buffer is full? */
			if (data->stdio.read_count == data->stdio.length) {
				break;
			}
			/* get another byte from the buffer */
			chr = buffer[copied];
			copied++;
			data->stdio.buffer[data->stdio.read_count] = chr;
			data->stdio.read_count++;
			/* caller wants full lines and we just completed one? */
			if ((data->stdio.delimiter == '\n') && (chr == '\n')) {
				sufficient = true;
				break;
			}
		} while (1);
		if (copied > 0) {
			/* consume the copied data (shift buffer) */
			memmove(buffer, buffer + copied, length + 1 - copied); /* NUL */
			length -= copied;
		}

		/* store away the remainder for later ... */

		/* need to resize our buffer? */
		if ((length > 0) && (sizeof(*ptr) + length) >= MemHandleSize(hdl)) {
			MemHandleUnlock(hdl);
			rc = MemHandleResize(hdl, sizeof(*ptr) + length + 256); /* "for fun" */
			ptr = MemHandleLock(hdl);
			if (rc != 0) {
				ptr->flags |= INPF_ERROR;
				length = 0; /* suppress storing */
			}
		}

		/* store the data, update fill count */
		if (length > 0) {
			memcpy(ptr->char_data, buffer, length);
			ptr->char_count = length;
		}

		/* release the temporary buffer */
		free(buffer);

	} while (0);

	/* signal flags to the caller */
	/* EOF sending NOT here, it's done above
	 * (deferred as long as necessary, disturbs apps otherwise) */
	if (ptr->flags & INPF_ERROR)
		data->stdio.had_error = 1;

	/* release the buffer */
	MemHandleUnlock(hdl);
	return;
}

/* the runtime injects requests here into the UI part of the app */
void app_add_event(const enum app_event_code code, union app_event_desc *data) {
	EventType ev;

	memset(&ev, 0, sizeof(ev));
	switch (code) {
	case APP_EVENT_EXIT:
		ev.eType = appStopEvent;
		EvtAddEventToQueue(&ev);
		break;
	case APP_EVENT_STDIN:
		/*
		 * collect the input data and
		 * then echo it to the screen
		 */
		app_input_get(code, data);
		/* FALLTHROUGH */
	case APP_EVENT_STDOUT:
	case APP_EVENT_STDERR:
		app_output_show(code, data);
		break;
	default:
		/* EMPTY */
		/* XXX actually a programmer's error */
		break;
	}

	return;
}

/*
 * the runtime may want to run the event handler, blocking or non blocking
 * (this currently only happens for the exit(3) routine, not for stdio)
 */
void app_run_event_handler(const int bWait) {
	EventType event;
	Err error;

	do {
		/* don't block on events when not allowed */
		if ((! bWait) && (! EvtEventAvail())) {
			return;
		}

		/* get another event and have it processed */
		EvtGetEvent(&event, evtWaitForever);
		if (event.eType == appStopEvent) {
			/* XXX TODO _what_ to do here to terminate the application? */
			break;
		}
		(void)(SysHandleEvent(&event) ||
			MenuHandleEvent(0, &event, &error) ||
			ApplicationHandleEvent(&event) ||
			FrmDispatchEvent(&event));
	} while (1);

	return;
}

/*
 * Palm application event handlers
 */

/* gadget related routines */

static void DrawOutputGadgetData(void) {
	output_dump(0);
	return;
}

static void DrawOutputGadget(void) {
	DrawOutputGadgetData();
	return;
}

static Boolean frmMainHandleGadget(FormGadgetTypeInCallback *g, UInt16 cmd, void *para) {
	Boolean handled;
	EventPtr ev;

	handled = false;
	switch (cmd) {
	case formGadgetDrawCmd:
		DrawOutputGadgetData();
		g->attr.visible = true; /* Form Manager requires this */
		handled = true;
		break;
	case formGadgetEraseCmd:
		WinEraseRectangle(&g->rect, 0);
		handled = true;
		break;
	case formGadgetHandleEventCmd:
		ev = /* (EventType *) */para;
#if 0	/* do we care about any event in the gadget? "scroll" with pen moves? */
		if (ev->eType == frmGadgetEnterEvent) { /* EMPTY */; }
		handled = true;
#endif
		break;
	case formGadgetDeleteCmd:
		if (g->data) {
			output_done((MemHandle)g->data);
		}
		handled = true;
		break;
	// default:
		/* EMPTY */
		break;
	}

	return(handled);
}

static void InitMainForm(FormPtr form) {
	struct inout_desc *ptr;
	UInt16 id, idx;

	/* start off with an empty input and output list */
	ptr = malloc(sizeof(*ptr));
	if (ptr == NULL)
		return;
	memset(ptr, 0, sizeof(*ptr));
	ptr->output = output_init();
	ptr->input = input_init();
	ptr->script = FetchSourceDatabase();

	/* register the chunk list with the gadget */
	id = ID_FLD_MAIN_OUTPUT;
	idx = FrmGetObjectIndex(form, id);
	FrmSetGadgetData(form, idx, ptr);
	FrmSetGadgetHandler(form, idx, frmMainHandleGadget);

	return;
}

static void DoneMainForm(FormPtr form) {
	UInt16 id, idx;
	struct inout_desc *ptr;

	/* get the chunk list from the gadget */
	id = ID_FLD_MAIN_OUTPUT;
	idx = FrmGetObjectIndex(form, id);
	ptr = FrmGetGadgetData(form, idx);

	/* unregister the chunk list with the gadget (needed?) */
	FrmSetGadgetData(form, idx, NULL);

	/* release the chunk list (clear the global var) */
	if (ptr != NULL) {
		if (ptr->output) {
			output_done(ptr->output);
		}
		if (ptr->input) {
			input_done(ptr->input);
		}
		if (ptr->script) {
			StoreSourceDatabase(ptr->script);
		}
		free(ptr);
	}

	return;
}

/* helper to dump Lua byte code for later analysis */

static void script_bytes_dump(const char *p, const int len) {
	const int bpl = 8;	/* bytes per line */

	const lu_byte *bp, *ep;
	char hex[40]; /* must be at least  4 * bpl + 1 */
	int idx;
	lu_byte b;
	char bb[3];

	bp = (lu_byte *)p;	/* unsigned, switch from char to byte */
	ep = bp + len;
	fprintf(stdout, "%d bytes at %p\n", len, p);
	while (bp < ep) {
		memset(hex, ' ', 4 * bpl);
		hex[4 * bpl] = '\0';
		for (idx = 0; idx < bpl; idx++) {
			if (bp + idx < ep) {
				b = bp[idx];
				snprintf(bb, sizeof(bb), "%02X", b);
				hex[3 * idx + 0] = bb[0];
				hex[3 * idx + 1] = bb[1];
				if (b < 0x20)
					b = '.';
				else if (b > 0x7F)
					b = '.';
				hex[3 * bpl + idx] = b;
			}
		}
		fprintf(stdout, "%s\n", hex);
		bp += bpl;
	}
}

static int script_get_opcode(const Instruction i) CSEC_LCODE;
static int script_get_opda(const Instruction i) CSEC_LCODE;
static int script_get_opdb(const Instruction i) CSEC_LCODE;
static int script_get_opdc(const Instruction i) CSEC_LCODE;
static int script_get_opdbx(const Instruction i) CSEC_LCODE;
static int script_get_opdsbx(const Instruction i) CSEC_LCODE;
static int script_get_test(const Instruction i) CSEC_LCODE;

static int script_get_opcode(const Instruction i) {
	return GET_OPCODE(i);
}

static int script_get_opda(const Instruction i) {
	return GETARG_A(i);
}

static int script_get_opdb(const Instruction i) {
	return GETARG_B(i);
}

static int script_get_opdc(const Instruction i) {
	return GETARG_C(i);
}

static int script_get_opdbx(const Instruction i) {
	return GETARG_Bx(i);
}

static int script_get_opdsbx(const Instruction i) {
	return GETARG_sBx(i);
}

static int script_get_test(const Instruction i) {
	OpCode opc; int arg;

	opc = script_get_opcode(i);
	arg = script_get_opda(i);
	arg = script_get_opdb(i);
	arg = script_get_opdc(i);
	arg = script_get_opdbx(i);
	arg = script_get_opdsbx(i);
	return(0);
}

static void DumpBytecode(const union Closure *code) CSEC_LCODE;
static void DumpBytecode(const union Closure *code) {
	int count;
	Instruction insn;
	OpCode opc;
	enum OpMode opm;
	const char *opn;
	int opdA, opdB, opdC;
	long opdBx, opdsBx;

	fprintf(stdout, "%ld instructions at %p\n", (long)code->l.p->sizecode, code->l.p->code);
	for (count = 0; count < code->l.p->sizecode; count++) {
		insn = code->l.p->code[count];
		script_get_test(insn);
		opc = GET_OPCODE(insn);
		opm = getOpMode(opc);
		opn = luaP_opname(opc);
		opdA = GETARG_A(insn);
		opdB = GETARG_B(insn);
		opdC = GETARG_C(insn);
		opdBx = GETARG_Bx(insn);
		opdsBx = GETARG_sBx(insn);

		fprintf(stdout, "%ld %08lX  %ld %ld  "
			, (long)count + 1, (long)insn
			, (long)opc, (long)opm
			);
		fprintf(stdout, "%s %ld"
			, luaP_opname(opc)
			, (long)opdA
			);
		switch (opm) {
		case iABC:
			fprintf(stdout, " %ld %ld", (long)opdB, (long)opdC);
			break;
		case iABx:
			fprintf(stdout, " %lu", (unsigned long)opdBx);
			break;
		case iAsBx:
			fprintf(stdout, " %ld", (long)opdsBx);
			break;
		}
		fprintf(stdout, "\n");

#if 0
		fprintf(stdout, "%d %08lX\n", count, (long)insn);
		fprintf(stdout, "  %02X", opc);
		fprintf(stdout, "  %02X", opdA);
		fprintf(stdout, "  %02X", opdB);
		fprintf(stdout, "  %02X", opdC);
		fprintf(stdout, "  %04lX", opdBx);
		fprintf(stdout, "  %04lX", opdsBx);
		fprintf(stdout, "\n");
#endif
	}
	fprintf(stdout, "end of bytecode dump\n");
}

/* routines to compile or run Lua scripts */

static int BytecodeScript(const char *script) {
	int rc;
	const char *errmsg;
	const char *dump;
	int dumplen;
	const union Closure *code;

	if ((script == NULL) || (*script == '\0')) {
		FrmCustomAlert(ID_FORM_FAILURE, "script text is empty", "", "");
		return(-1);
	}

	/* clear stack */
	lua_settop(L, 0);

	/* compile the source */
	rc = luaL_loadstring(L, script);
	if (rc != 0) {
		errmsg = luaL_checkstring(L, -1);
		if (! errmsg) errmsg = "???";
		FrmCustomAlert(ID_FORM_FAILURE, "load failure", errmsg, "");
		return(-1);
	}

	/* make sure the result is a (Lua) function */
	if (lua_type(L, -1) != LUA_TFUNCTION) {
		FrmCustomAlert(ID_FORM_FAILURE, "result not a (Lua) function", "", "");
		return(-1);
	}

	/* call string.dump(), get string length and bytes */
	lua_getglobal(L, "string");	/* bytes table */
	lua_getfield(L, -1, "dump");	/* bytes table field */
	lua_pushvalue(L, -3);		/* bytes table field bytes */
	lua_pcall(L, 1, 1, 0);		/* bytes table dump */
	dump = luaL_checkstring(L, -1);
	dumplen = lua_objlen(L, -1);
	script_bytes_dump(dump, dumplen);
	lua_pop(L, 2);			/* bytes */

	/* XXX
	 * this is a rather incomplete view, it only lists "level 0" instructions
	 * and lacks the constants, nested functions and upvalues; to get it right
	 * we had to mimic luac(1) behaviour -- luaU_dump() or luaU_print(), the
	 * latter is PrintFunction() in lua/src/print.c
	 */

	/* print the header for the instructions */
	code = lua_topointer(L, -1);
	DumpBytecode(code);

	/* clear the stack, run a GC cycle */
	lua_settop(L, 0);
	lua_gc(L, LUA_GCCOLLECT, 0);
	return(0);
}

static int RunScript(const char *script) {
	int wantret;
	int len;
	char *src;
	int rc;
	const char *errmsg;

	if ((script == NULL) || (*script == '\0')) {
		FrmCustomAlert(ID_FORM_FAILURE, "script text is empty", "", "");
		return(-1);
	}

	/* clear stack */
	lua_settop(L, 0);

	/* source code starts with "=..."? */
	wantret = 0;
	src = (char *)script; /* UNCONST */
	if (*script == '=') {
		wantret = 1;
		len = strlen("return ") + strlen(script) + 1;
		src = malloc(len);
		if (src == NULL) {
			FrmCustomAlert(ID_FORM_FAILURE, "out of memory", "", "");
			return(-1);
		}
		snprintf(src, len, "return %s", script + 1);
	}

	/* compile the source */
	rc = luaL_loadstring(L, src);
	if (src != script)
		free(src);
	if (rc != 0) {
		errmsg = luaL_checkstring(L, -1);
		if (! errmsg) errmsg = "???";
		FrmCustomAlert(ID_FORM_FAILURE, "load failure", errmsg, "");
		return(-1);
	}

	/* run the code, alert upon errors, optionally echo results, clear stack */
	rc = lua_pcall(L, 0, LUA_MULTRET, 0);
	if (rc != 0) {
		errmsg = luaL_checkstring(L, -1);
		if (! errmsg) errmsg = "???";
		FrmCustomAlert(ID_FORM_FAILURE, "run failure", errmsg, "");
	}
	if ((rc == 0) && (lua_gettop(L) > 0) && (wantret)) {
		lua_getglobal(L, "print");
		lua_insert(L, 1);
		lua_pcall(L, lua_gettop(L)-1, 0, 0);
	}
	lua_settop(L, 0);

	/* run the garbage collector before idling */
	lua_gc(L, LUA_GCCOLLECT, 0);
	return((rc == 0) ? 0 : -1);
}

/* scrollbar/textfield handling routines */

/* from O'Reilly, pp 273 */
static void SetFieldTextFromHandle(FieldPtr field, MemHandle newHandle, Boolean redraw) {
	MemHandle oldHandle;

	oldHandle = FldGetTextHandle(field);

	FldSetTextHandle(field, newHandle);
	if (redraw)
		FldDrawField(field);

	if (oldHandle)
		MemHandleFree(oldHandle);
}

/* from O'Reilly, pp 279 */
static void UpdateScrollbar(FormPtr form, UInt16 fieldID, UInt16 scrollbarID) {
	ScrollBarPtr scroll;
	FieldPtr field;
	UInt16 currentPosition, textHeight, fieldHeight, maxValue;

	field = FrmGetObjectPtr(form, FrmGetObjectIndex(form, fieldID));
	FldGetScrollValues(field, &currentPosition, &textHeight, &fieldHeight);
	if (textHeight > fieldHeight)
		maxValue = textHeight - fieldHeight;
	else if (currentPosition)
		maxValue = currentPosition;
	else
		maxValue = 0;

	scroll = FrmGetObjectPtr(form, FrmGetObjectIndex(form, scrollbarID));
	SclSetScrollBar(scroll, currentPosition, 0, maxValue, fieldHeight - 1);
}

/* from O'Reilly, pp 279 */
static void ScrollLines(FormPtr form, UInt16 fieldID, UInt16 scrollbarID, Int16 numLinesToScroll, Boolean redraw) {
	FormPtr frm = FrmGetActiveForm(); /* XXX isn't this always what's in "form"? */
	FieldPtr field;

	field = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, fieldID));
	if (numLinesToScroll < 0)
		FldScrollField(field, -numLinesToScroll, winUp);
	else
		FldScrollField(field, numLinesToScroll, winDown);

	if ((FldGetNumberOfBlankLines(field) && numLinesToScroll < 0) || redraw)
		UpdateScrollbar(form, fieldID, scrollbarID);
}

/* from O'Reilly, pp 279 */
static void PageScroll(FormPtr form, UInt16 fieldID, UInt16 scrollbarID, WinDirectionType direction) {
	FieldPtr field;

	field = FrmGetObjectPtr(form, FrmGetObjectIndex(form, fieldID));
	if (FldScrollable(field, direction)) {
		Int16 linesToScroll = FldGetVisibleLines(field) - 1;
		if (direction == winUp)
			linesToScroll = -linesToScroll;
		ScrollLines(form, fieldID, scrollbarID, linesToScroll, true);
	}
}

/* routines filling the source editor from DB resources or memo files */

static void script_add_string(FieldPtr fld, const char *txt, const int len) {
	UInt16 inspt;

	/* insert the string at the current cursor position, highlight the new text */
	inspt = FldGetInsPtPosition(fld);
	/* bool */ FldInsert(fld, txt, len);
	FldSetSelection(fld, inspt, inspt + len);
	/* XXX can it ever happen that "insertion point" and "selection" differ? */
}

static struct {
	Int16 count;
	UInt32 *recno;
	char **names;
} memo_list = {
	0, NULL, NULL,
};

static void DoneMemoList(void); /* forward decl */

static void InitMemoList(void) {
	UInt16 mode;
	DmOpenRef memoDB;
	UInt16 cat, total, i;
	UInt16 recno;
	MemHandle rechdl;
	char *name;
	int namelen, displen;
	int wantit;

	/* open memo DB, determine record count */
	mode = dmModeReadOnly;
	if (PrefGetPreference(prefShowPrivateRecords) != hidePrivateRecords)
		mode |= dmModeShowSecret;
	memoDB = DmOpenDatabaseByTypeCreator('DATA', 'memo', mode);
	if (! memoDB) {
		return;
	}

	cat = dmAllCategories;
	total = DmNumRecordsInCategory(memoDB, cat);

	/* allocate recno[] and names[] tables */
	if (total == 0) {
		DmCloseDatabase(memoDB);
		return;
	}
	memo_list.recno = malloc(total * sizeof(memo_list.recno[0]));
	if (memo_list.recno != NULL)
		memset(memo_list.recno, 0, total * sizeof(memo_list.recno[0]));
	memo_list.names = malloc(total * sizeof(memo_list.names[0]));
	if (memo_list.names != NULL)
		memset(memo_list.names, 0, total * sizeof(memo_list.names[0]));
	if ((memo_list.recno == NULL) || (memo_list.names == NULL)) {
		DmCloseDatabase(memoDB);
		DoneMemoList();
		return;
	}

	/* run through memo DB, add (selected?) "filenames" */
	for (i = 0, recno = 0; i < total; i++, recno++) {

		/* get a the next memo item */
		rechdl = DmQueryNextInCategory(memoDB, &recno, cat);
		if (! rechdl)
			break;
		name = MemHandleLock(rechdl);
		if (! name) {
			break;
		}
		namelen = strlen(name);

		/* determine the name we would display for that memo */
		for (displen = 0; displen <= namelen; displen++) {
			if (name[displen] == '\0')
				break;
			if (name[displen] == '\n')
				break;
		}

		/* check whether we want to list that memo */
		wantit = 1;

		/* optionally trim off "-- " leaders */
		if ((1) && (strncmp(name, "--", strlen("--")) == 0)) {
			displen -= strlen("--");
			namelen -= strlen("--");
			name += strlen("--");
			while (*name == ' ') {
				displen--;
				namelen--;
				name++;
			}
		}

		/* add the item to our list */
		if (wantit) {
			memo_list.recno[memo_list.count] = recno;
			memo_list.names[memo_list.count] = malloc(displen + 1);
			/* just don't display what cannot be alloced */
			if (memo_list.names[memo_list.count] != NULL) {
				snprintf(memo_list.names[memo_list.count], displen + 1, "%s", name);
				memo_list.count++;
			}
		}

		/* release the record data */
		MemHandleUnlock(rechdl);
	}

	/* close the DB, we're done */
	DmCloseDatabase(memoDB);

	return;
}

static void DoneMemoList(void) {
	Int16 idx;

	if (memo_list.names != NULL) {
		for (idx = 0; idx < memo_list.count; idx++) {
			if (memo_list.names[idx] != NULL) {
				free(memo_list.names[idx]);
			}
			memo_list.names[idx] = NULL;
		}
		free(memo_list.names);
		memo_list.names = NULL;
	}
	if (memo_list.recno != NULL) {
		free(memo_list.recno);
		memo_list.recno = NULL;
	}
	memo_list.count = 0;

	return;
}

static int LoadStringResource(UInt16 id, FieldPtr fld) {
	MemHandle hdl;
	const char *txt;

	/* replace menu item IDs with their associated text resource ID */
	switch (id) {
	case ID_MITEM_SOURCE_FILE_STURTLE:
		id = ID_STR_SAMPLE_TURTLE;
		break;
	case ID_MITEM_SOURCE_FILE_SBITMAP:
		id = ID_STR_SAMPLE_BITMAP;
		break;
	case ID_MITEM_SOURCE_FILE_SHOUSE:
		id = ID_STR_SAMPLE_HOUSE;
		break;
	default:
		/* EMPTY */
		break;
	}

	/* lookup the string resource */
	hdl = DmGetResource('tSTR', id);
	if (! hdl) {
		FrmCustomAlert(ID_FORM_FAILURE, "string resource not found", "", "");
		return(-1);
	}

	txt = MemHandleLock(hdl);
	script_add_string(fld, txt, strlen(txt));

	/* release the string resources */
	MemHandleUnlock(hdl);
	DmReleaseResource(hdl);
	return(0);
}

static int LoadMemoItem(const UInt16 recno, FieldPtr fld) {
	DmOpenRef memoDB;
	MemHandle hdl;
	char *txt;
	int rc;

	memoDB = NULL;
	hdl = 0;
	txt = NULL;

	memoDB = DmOpenDatabaseByTypeCreator('DATA', 'memo', dmModeReadOnly);
	if (memoDB != NULL)
		hdl = DmQueryRecord(memoDB, recno);
	if (hdl)
		txt = MemHandleLock(hdl);

	if (txt != NULL) {
		script_add_string(fld, txt, strlen(txt));
		rc = 0;
	} else {
		FrmCustomAlert(ID_FORM_FAILURE, "loading from memo failed", "", "");
		rc = -1;
	}

	if (hdl)
		MemHandleUnlock(hdl);
	if (memoDB != NULL)
		DmCloseDatabase(memoDB);
	return(rc);
}

static int SaveMemoItem(UInt16 recno, const Boolean mknew, FieldPtr fld) {
	char *txtptr;
	UInt32 txtlen;
	DmOpenRef memoDB;
	MemHandle rechdl;
	char *recptr;
	Err err;
	int rc;

	txtptr = FldGetTextPtr(fld);
	txtlen = FldGetTextLength(fld) + 1;	/* yes, save NUL too */
	memoDB = NULL;
	rechdl = 0;
	rc = -1;

	memoDB = DmOpenDatabaseByTypeCreator('DATA', 'memo', dmModeReadWrite);
	if (memoDB != NULL) {
		if (mknew) {
			rechdl = DmNewRecord(memoDB, &recno, txtlen);
		} else {
			rechdl = DmGetRecord(memoDB, recno);
			if (rechdl)
				DmResizeRecord(memoDB, recno, txtlen);
		}
	}

	if (! rechdl) {
		FrmCustomAlert(ID_FORM_FAILURE, "could not open memo record", "", "");
	} else {
		recptr = MemHandleLock(rechdl);
		err = DmWrite(recptr, 0, txtptr, txtlen);
		MemHandleUnlock(rechdl);
		if (err) {
			FrmCustomAlert(ID_FORM_FAILURE, "error writing to memo record", "", "");
		} else {
			rc = 0;
		}
	}

	if (rechdl)
		DmReleaseRecord(memoDB, recno, true);
	if (memoDB != NULL)
		DmCloseDatabase(memoDB);

	return(rc);
}

static void DrawMemoListItem(Int16 item, RectangleType *bounds, Char **itemsTxt) {
	char *s;

	s = 0;
	if (item >= memo_list.count)
		return;
	s = memo_list.names[item];
	WinDrawChars(s, StrLen(s), bounds->topLeft.x, bounds->topLeft.y);
	return;
}

static UInt16 RunMemoSelect(FormPtr form, int *mknew) {
	UInt16 recno;
	FormPtr selform;
	UInt16 id, idx;
	ListPtr lst;
	Int16 selidx;

	/* XXX what's a value to signal "none"? */
	recno = dmMaxRecordIndex;

	/* create the form */
	selform = FrmInitForm(ID_FORM_FILESEL);

	/* XXX if mknew != NULL, enable the "new" button */
	id = ID_BTN_FILESEL_MKNEW;
	idx = FrmGetObjectIndex(selform, id);
	if (mknew != NULL) {
		*mknew = 0;
		CtlSetEnabled(FrmGetObjectPtr(selform, idx), true);
	} else {
		CtlSetEnabled(FrmGetObjectPtr(selform, idx), false);
	}

	/* get a handle to the list item */
	id = ID_LST_FILESEL_FILES;
	idx = FrmGetObjectIndex(selform, id);
	lst = FrmGetObjectPtr(selform, idx);
	FrmSetFocus(form, idx);

	/* determine the list of items, register the draw CB */
	InitMemoList();
	LstSetDrawFunction(lst, DrawMemoListItem);
	LstSetListChoices(lst, NULL, memo_list.count);
	LstSetSelection(lst, noListSelection);

	/* run the form, actually no separate event handler needed */
	id = FrmDoDialog(selform);

	/* determine the record number when an item was selected */
	if (id == ID_BTN_FILESEL_OK) {
		/* a memo was selected */
		selidx = LstGetSelection(lst);
		if ((selidx != noListSelection) && (selidx < memo_list.count))
			recno = memo_list.recno[selidx];
	}
	/* tell the caller that a new item was requested */
	if (id == ID_BTN_FILESEL_MKNEW) {
		*mknew = 1;
	}

	/* release the list of memos, destroy the selector form */
	DoneMemoList();
	FrmDeleteForm(selform);

	return(recno);
}

/* source editor form event handling routine */

static Boolean frmSourceHandleEvent(EventPtr event) {
	Boolean handled;
	UInt16 id, idx;
	FormPtr form;
	FieldPtr fld;
	char *src, *dest;
	MemHandle hdl;
	struct inout_desc *desc;
	WChar c;
	UInt16 recno;
	int mknew;

	handled = false;
	form = FrmGetActiveForm();
	switch (event->eType) {
	case frmOpenEvent:	/* should not happen with FrmDoDialog() */
		break;
	case frmCloseEvent:	/* does this happen before the "field close"? */
		// FrmCustomAlert(ID_FORM_INFORMATION, "Source form", "close event", "");
		break;
	case menuEvent:
		id = event->data.menu.itemID;
		switch (id) {
		case ID_MITEM_SOURCE_FILE_STURTLE:
		case ID_MITEM_SOURCE_FILE_SBITMAP:
		case ID_MITEM_SOURCE_FILE_SHOUSE:
			idx = FrmGetObjectIndex(form, ID_FLD_SOURCE_INPUT);
			fld = FrmGetObjectPtr(form, idx);
			LoadStringResource(id, fld);
			handled = true;
			break;
		case ID_MITEM_SOURCE_FILE_LMEMO:
			idx = FrmGetObjectIndex(form, ID_FLD_SOURCE_INPUT);
			fld = FrmGetObjectPtr(form, idx);
			recno = RunMemoSelect(form, NULL);
			handled = true;
			if (recno == dmMaxRecordIndex)
				break;
			LoadMemoItem(recno, fld);
			break;
		case ID_MITEM_SOURCE_FILE_SMEMO:
			idx = FrmGetObjectIndex(form, ID_FLD_SOURCE_INPUT);
			fld = FrmGetObjectPtr(form, idx);
			recno = RunMemoSelect(form, &mknew);
			handled = true;
			if ((recno == dmMaxRecordIndex) && (! mknew))
				break;
			SaveMemoItem(recno, mknew, fld);
			break;
		case ID_MITEM_SOURCE_HELP_HELP:
			FrmHelp(ID_LBL_SOURCE_HELP);
			handled = true;
			break;
		}
		break;
	case ctlSelectEvent:
		id = event->data.ctlSelect.controlID;
		switch (id) {
		case ID_BTN_SOURCE_EXEC:
		case ID_BTN_SOURCE_BYTE:
		case ID_BTN_SOURCE_CLOSE:
			// FrmCustomAlert(ID_FORM_INFORMATION, "Source form", "ctl select event", "button");
			/* prevent the field from freeing the script text */
			idx = FrmGetObjectIndex(form, ID_FLD_SOURCE_INPUT);
			fld = FrmGetObjectPtr(form, idx);
			src = FldGetTextPtr(fld);
			if (src == NULL) {
				FrmCustomAlert(ID_FORM_FAILURE, "no text to copy", "", "");
				break;
			}
			hdl = MemHandleNew(strlen(src) + 1);
			if (! hdl) {
				FrmCustomAlert(ID_FORM_FAILURE, "no memory avail", "", "");
				break;
			}
			dest = MemHandleLock(hdl);
			StrCopy(dest, src);
			MemHandleUnlock(hdl);
			form = FrmGetFormPtr(ID_FORM_MAIN);
			idx = FrmGetObjectIndex(form, ID_FLD_MAIN_OUTPUT);
			desc = FrmGetGadgetData(form, idx);
			desc->script = hdl;
			// SetFieldTextFromHandle(fld, NULL, false);
			/* NOT handled, have the form closed etc */
			break;
		default:
			/* EMPTY */
			break;
		}
		break;
	case fldChangedEvent:
		UpdateScrollbar(form, ID_FLD_SOURCE_INPUT, ID_SCR_SOURCE_INPUT);
		handled = true;
		break;
	case sclRepeatEvent:
		ScrollLines(form, ID_FLD_SOURCE_INPUT, ID_SCR_SOURCE_INPUT
			, event->data.sclRepeat.newValue - event->data.sclRepeat.value
			, false);
		/* NOT handled, otherwise won't repeat */
		break;
	case keyDownEvent:
		c = event->data.keyDown.chr;
	#if 0	/* needed for 2-byte systems (like Japanese) */
		if (! TxtGlueCharIsVirtual(event->data.keyDown.modifiers, c))
			break;
	#endif
		if (c == vchrPageUp) {
			PageScroll(form, ID_FLD_SOURCE_INPUT, ID_SCR_SOURCE_INPUT, winUp);
			handled = true;
		} else if (c == vchrPageDown) {
			PageScroll(form, ID_FLD_SOURCE_INPUT, ID_SCR_SOURCE_INPUT, winDown);
			handled = true;
		}
		break;
	default:
		/* EMPTY */
		break;
	}
	return(handled);
}

/* routine to run the source editor form as a dialog */

static void RunSourceForm(FormPtr form) {
	UInt16 id, idx;
	struct inout_desc *desc;
	FormPtr src;
	FieldPtr fld;
	MemHandle hdl;
	int rc;
	char *script;

	/* get a handle to the script text */
	id = ID_FLD_MAIN_OUTPUT;
	idx = FrmGetObjectIndex(form, id);
	if (! idx)
		return;
	desc = FrmGetGadgetData(form, idx);
	if (desc == NULL)
		return;

	/* create the source form */
	id = ID_FORM_SOURCE;
	src = FrmInitForm(id);

	/* load the text into the  source field */
	id = ID_FLD_SOURCE_INPUT;
	idx = FrmGetObjectIndex(src, id);
	fld = FrmGetObjectPtr(src, idx);
	hdl = desc->script; desc->script = NULL;
	SetFieldTextFromHandle(fld, hdl, false);
	FrmSetFocus(src, idx);
	UpdateScrollbar(src, ID_FLD_SOURCE_INPUT, ID_SCR_SOURCE_INPUT);

	/* run the form (with its own event handler) */
	FrmSetEventHandler(src, frmSourceHandleEvent);
	rc = FrmDoDialog(src);
	FrmSetEventHandler(form, frmMainHandleEvent);

	// /* detach the script text from the input field */
	// SetFieldTextFromHandle(fld, NULL, false);

	/* destroy the form */
	FrmDeleteForm(src);

	/* run or compile the script if requested */
	if (rc == ID_BTN_SOURCE_EXEC) {
		hdl = desc->script;
		if (hdl) {
			script = MemHandleLock(hdl);
			RunScript(script);
			MemHandleUnlock(hdl);
		}
	}
	if (rc == ID_BTN_SOURCE_BYTE) {
		hdl = desc->script;
		if (hdl) {
			script = MemHandleLock(hdl);
			BytecodeScript(script);
			MemHandleUnlock(hdl);
		}
	}
}

/* routine to run the setup form as a dialog */

static void RunSetupForm(FormPtr form) {
	UInt16 id;
	FormPtr setup;
	Boolean bClearOutput, bRedrawOutput;

	/* create the setup form */
	id = ID_FORM_SETUP;
	setup = FrmInitForm(id);

	/* load current values into the controls */
	InitSetupForm(setup);

	/* run the form (no own event handler needed) */
	id = FrmDoDialog(setup);

	/* if case of "Apply" save the values */
	bClearOutput = false;
	bRedrawOutput = false;
	if (id == ID_BTN_SETUP_APPLY) {
		SaveSetupForm(setup, &bClearOutput, &bRedrawOutput);
	}

	/* call that routine although it's empty */
	DoneSetupForm(setup);

	/* destroy the form */
	FrmDeleteForm(setup);

	/* trigger redraw or erase output when necessary */
	if (bClearOutput)
		output_clear();
	if (bClearOutput || bRedrawOutput)
		FrmUpdateForm(ID_FORM_MAIN, frmRedrawUpdateCode);
}

static void RunRuntimeForm(FormPtr form) {
	UInt16 id;
	FormPtr runtime;

	id = ID_FORM_RUNTIME;
	runtime = FrmInitForm(id);
	InitRuntimeForm(runtime);
	FrmSetEventHandler(runtime, frmRuntimeHandleEvent);
	id = FrmDoDialog(runtime);
	FrmSetEventHandler(form, frmMainHandleEvent);
	DoneRuntimeForm(runtime);
	FrmDeleteForm(runtime);
}

/* main form event handling routine */

static Boolean
frmMainHandleEvent(EventPtr event) {
	Boolean handled;
	FormPtr form;
	UInt16 id, idx;
	FieldPtr fld;
	const char *script;
	int rc;
	FormPtr about;
	ControlType *ctl;

	handled = false;
	form = FrmGetActiveForm();
	switch (event->eType) {
	case frmOpenEvent:
		// FrmCustomAlert(ID_FORM_INFORMATION, "main form open", "", "");
		InitMainForm(form);
		FrmDrawForm(form);
		DrawOutputGadget();
		id = ID_FLD_MAIN_SCRIPT;
		idx = FrmGetObjectIndex(form, id);
		FrmSetFocus(form, idx);
		handled = true;
		break;
	case frmUpdateEvent:
		// FrmCustomAlert(ID_FORM_INFORMATION, "main form update", "", "");
		FrmDrawForm(form);
		DrawOutputGadget();
		handled = true;
		break;
	case frmSaveEvent:
		// FrmCustomAlert(ID_FORM_INFORMATION, "main form save", "", "");
		/* NOT handled */
		break;
	case frmCloseEvent:
		// FrmCustomAlert(ID_FORM_INFORMATION, "main form close", "", "");
		DoneMainForm(form);
		/* NOT handled, have the default action be done */
		break;
	case menuEvent:
		id = event->data.menu.itemID;
		switch (id) {
/* XXX TODO these are all fake actions (development aid) */
		char buff[20];
		RectangleType bounds;
		case ID_MITEM_MAIN_FILE_NEW:
			/* "output" test */
			fputs("output", stdout);
			fputs("error", stderr);
			handled = true;
			break;
		case ID_MITEM_MAIN_FILE_OPEN:
			/* "input" test */
			fgets(buff, sizeof(buff), stdin);
			handled = true;
			break;
		case ID_MITEM_MAIN_FILE_CLOSE:
			/* "refresh" trigger */
			idx = FrmGetObjectIndex(form, ID_FLD_MAIN_OUTPUT);
			FrmGetObjectBounds(form, idx, &bounds);
			WinEraseRectangle(&bounds, 0);
			output_dump(0);
			handled = true;
			break;
		case ID_MITEM_MAIN_FILE_RUN:
			RunRuntimeForm(form);
			handled = true;
			break;
		case ID_MITEM_MAIN_FILE_QUIT:
			/* "erase" trigger */
			idx = FrmGetObjectIndex(form, ID_FLD_MAIN_OUTPUT);
			output_clear();
			output_dump(0);
			FrmGetObjectBounds(form, idx, &bounds);
			WinEraseRectangle(&bounds, 0);
			handled = true;
			break;
/* XXX TODO up to here fake actions (development aid) */
		case ID_MITEM_MAIN_SETUP_CONFIG:
			RunSetupForm(form);
			handled = true;
			break;
		case ID_MITEM_MAIN_HELP_HELP:
			FrmHelp(ID_LBL_MAIN_HELP);
			handled = true;
			break;
		case ID_MITEM_MAIN_HELP_ABOUT:
			about = FrmInitForm(ID_FORM_ABOUT);
			FrmDoDialog(about);
			FrmDeleteForm(about);
			handled = true;
			break;
		default:
			/* EMPTY */
			break;
		}
		break;
	case ctlSelectEvent:
		id = event->data.ctlSelect.controlID;
		switch (id) {
		case ID_BTN_MAIN_EXEC:
		case ID_BTN_MAIN_BYTE:
			/* get the text field content */
			idx = FrmGetObjectIndex(form, ID_FLD_MAIN_SCRIPT);
			fld = FrmGetObjectPtr(form, idx);
			script = FldGetTextPtr(fld);
			/* run the script code */
			if (id == ID_BTN_MAIN_EXEC) {
				rc = RunScript(script);
			} else if (id == ID_BTN_MAIN_BYTE) {
				rc = BytecodeScript(script);
			} else {
				rc = -1;
			}
			/* have all input selected to continue */
			if (rc == 0) {
				UInt16 len;
				len = FldGetTextLength(fld);
				FldSetSelection(fld, 0, len);
			}
			handled = true;
			break;
		case ID_BTN_MAIN_SOURCE:
			RunSourceForm(form);
			handled = true;
			break;
		default:
			/* EMPTY */
			break;
		}
		break;
	case sclRepeatEvent:
		output_scroll_lines(event->data.sclRepeat.newValue - event->data.sclRepeat.value, false);
		/* NOT handled, otherwise won't repeat */
		break;
	case keyDownEvent:
		switch (event->data.keyDown.chr) {
		case vchrPageUp:
			output_scroll_page(winUp);
			handled = true;
			break;
		case vchrPageDown:
			output_scroll_page(winDown);
			handled = true;
			break;
		case '\n':
		case '\r':
			idx = FrmGetObjectIndex(form, ID_BTN_MAIN_EXEC);
			ctl = FrmGetObjectPtr(form, idx);
			CtlHitControl(ctl);
			break;
		case '\t':
			idx = FrmGetObjectIndex(form, ID_BTN_MAIN_BYTE);
			ctl = FrmGetObjectPtr(form, idx);
			CtlHitControl(ctl);
			break;
		}
	default:
		/* EMPTY */
		break;
	}
	return(handled);
}

/* setup form related routines */

/* load preferences into the form objects */
static void InitSetupForm(FormPtr form) {
	UInt16 id, idx;
	FieldPtr field;
	MemHandle txthdl;
	char *txtptr;
	int len;

	/* the "output markup" button group */
	switch (preferences.output.markup) {
	case OMK_NONE : id = ID_BTN_SETUP_OUTPUT_MARKUP_NONE ; break;
	case OMK_MONO : id = ID_BTN_SETUP_OUTPUT_MARKUP_MONO ; break;
	case OMK_COLOR: id = ID_BTN_SETUP_OUTPUT_MARKUP_COLOR; break;
	}
	idx = FrmGetObjectIndex(form, id);
	FrmSetControlValue(form, idx, true);

	/* XXX TODO check errors */
	/* max output lines */
	id = ID_FLD_SETUP_MAXLINES;
	idx = FrmGetObjectIndex(form, id);
	field = FrmGetObjectPtr(form, idx);
	len = 8;
	txthdl = MemHandleNew(len);
	txtptr = MemHandleLock(txthdl);
	snprintf(txtptr, len, "%d", preferences.output.maxlines);
	MemHandleUnlock(txthdl);
	SetFieldTextFromHandle(field, txthdl, false);

	/* XXX TODO check errors */
	/* max output bytes */
	id = ID_FLD_SETUP_MAXBYTES;
	idx = FrmGetObjectIndex(form, id);
	field = FrmGetObjectPtr(form, idx);
	len = 8;
	txthdl = MemHandleNew(len);
	txtptr = MemHandleLock(txthdl);
	snprintf(txtptr, len, "%d", preferences.output.maxbytes);
	MemHandleUnlock(txthdl);
	SetFieldTextFromHandle(field, txthdl, false);

	/* XXX TODO high res mode */
	id = ID_CHK_SETUP_HIRES;
	idx = FrmGetObjectIndex(form, id);
	FrmSetControlValue(form, idx, (preferences.output.usehires) ? true : false);

	/* the "capability diag" switch */
	id = ID_CHK_SETUP_DIAG_CAPA;
	idx = FrmGetObjectIndex(form, id);
	FrmSetControlValue(form, idx, (preferences.diag.capa) ? true : false);

	return;
}

/* save changed preferences to variables */
static void SaveSetupForm(FormPtr form, Boolean *bClear, Boolean *bRedraw) {
	UInt16 id, idx;
	Boolean flg;
	FieldPtr fld;
	const char *txt;
	enum output_markup_kind saveMarkup;
	int saveInt;
	Boolean saveBool;

	if (bClear != NULL)
		*bClear = false;
	if (bRedraw != NULL)
		*bRedraw = false;

	/* the "output markup" group, one of them is active */
	saveMarkup = preferences.output.markup;

	id = ID_BTN_SETUP_OUTPUT_MARKUP_NONE;
	idx = FrmGetObjectIndex(form, id);
	flg = FrmGetControlValue(form, idx);
	if (flg)
		preferences.output.markup = OMK_NONE;

	id = ID_BTN_SETUP_OUTPUT_MARKUP_MONO;
	idx = FrmGetObjectIndex(form, id);
	flg = FrmGetControlValue(form, idx);
	if (flg)
		preferences.output.markup = OMK_MONO;

	id = ID_BTN_SETUP_OUTPUT_MARKUP_COLOR;
	idx = FrmGetObjectIndex(form, id);
	flg = FrmGetControlValue(form, idx);
	if (flg)
		preferences.output.markup = OMK_COLOR;

	if ((bRedraw != NULL) && (saveMarkup != preferences.output.markup))
		*bRedraw = true;

	/* max output lines, max output bytes */
	saveInt = preferences.output.maxlines;

	id = ID_FLD_SETUP_MAXLINES;
	idx = FrmGetObjectIndex(form, id);
	fld = FrmGetObjectPtr(form, idx);
	txt = FldGetTextPtr(fld);
	preferences.output.maxlines = strtol(txt, NULL, 0);
	if (preferences.output.maxlines < 50) {
		/* XXX alert the user of this adjusting? */
		preferences.output.maxlines = 50;
	}

	if ((bClear != NULL) && (saveInt != preferences.output.maxlines))
		*bClear = true;

	saveInt = preferences.output.maxbytes;

	id = ID_FLD_SETUP_MAXBYTES;
	idx = FrmGetObjectIndex(form, id);
	fld = FrmGetObjectPtr(form, idx);
	txt = FldGetTextPtr(fld);
	preferences.output.maxbytes = strtol(txt, NULL, 0);
	if (preferences.output.maxbytes < 4000) {
		/* XXX alert the user of this adjusting? */
		preferences.output.maxbytes = 4000;
	}

	if ((bClear != NULL) && (saveInt != preferences.output.maxbytes))
		*bClear = true;

	/* hires checkbox */
	saveBool = preferences.output.usehires;

	id = ID_CHK_SETUP_HIRES;
	idx = FrmGetObjectIndex(form, id);
	flg = FrmGetControlValue(form, idx);
	preferences.output.usehires = (flg) ? true : false;

	if ((bClear != NULL) && (saveBool != preferences.output.usehires))
		*bClear = true;

	/* the "capabilities diag" switch */
	id = ID_CHK_SETUP_DIAG_CAPA;
	idx = FrmGetObjectIndex(form, id);
	flg = FrmGetControlValue(form, idx);
	preferences.diag.capa = (flg) ? 1 : 0;

	return;
}

static void DoneSetupForm(FormPtr form) {
	/* EMPTY */
	return;
}

/* runtime form related routines (test stuff) */

static const luaL_reg palmapp[] = {
	{ NULL, NULL, },
};

static void runtime_open_func(void) {
	const char *s1, *s2, *s3;
	Instruction i;

	/* print a few parameters which influence our code */
	/* XXX is assert() available? */
	assert(true);
	printf("INT_MAX[%ld]\n", (long)INT_MAX);
	printf("MAX_INT[%ld]\n", (long)MAX_INT);
	printf("LUAI_BITSINT[%ld]\n", (long)LUAI_BITSINT);
	printf("sizeof(LUAI_UINT32)[%ld]\n", (long)sizeof(LUAI_UINT32));
	printf("sizeof(LUAI_INT32)[%ld]\n", (long)sizeof(LUAI_INT32));
	printf("sizeof(LUAI_MAXINT32)[%ld]\n", (long)sizeof(LUAI_MAXINT32));
	printf("sizeof(LUAI_UMEM)[%ld]\n", (long)sizeof(LUAI_UMEM));
	printf("sizeof(LUAI_MEM)[%ld]\n", (long)sizeof(LUAI_MEM));

#if 0
	i = 0xffffffffL;
#else
	i = 0;
	SET_OPCODE(i, 1);
	SETARG_A(i, 2);
	SETARG_B(i, 3);
	SETARG_C(i, 4);
#endif
	printf("insn[%ld]\n", i);
	printf("opcode[%ld]\n", (Instruction)GET_OPCODE(i));
	printf("arg a[%ld]\n", (Instruction)GETARG_A(i));
	printf("arg b[%ld]\n", (Instruction)GETARG_B(i));
	printf("arg c[%ld]\n", (Instruction)GETARG_C(i));
	printf("arg bx[%ld]\n", (Instruction)GETARG_Bx(i));
	printf("arg sbx[%ld]\n", (Instruction)GETARG_sBx(i));
	printf("arg c[%ld]\n", (Instruction)GETARG_C(i));

	lua_getglobal(L, "runtime");
	if (! lua_isnil(L, -1)) {
		lua_getfield(L, -1, "key1");
		s1 = luaL_checkstring(L, -1);

		lua_getfield(L, -2, "key2");
		s2 = luaL_checkstring(L, -1);

		lua_getfield(L, -3, "key3");
		s3 = luaL_checkstring(L, -1);

		FrmCustomAlert(ID_FORM_INFORMATION, s1, s2, s3);

		lua_pop(L, 4);
		return;
	}

	lua_newtable(L);
	// luaL_register(L, "palmapp", palmapp);

	lua_pushstring(L, "val1");
	lua_setfield(L, -2, "key1");
	lua_pushstring(L, "val2");
	lua_setfield(L, -2, "key2");
	lua_pushstring(L, "val3");
	lua_setfield(L, -2, "key3");
	lua_pushnumber(L, 23);
	lua_setfield(L, -2, "key4");
	lua_pushnumber(L, 42);
	lua_setfield(L, -2, "key5");
	lua_pushstring(L, "val6");
	lua_setfield(L, -2, "key2"); /* YES, key2 again */

	lua_setglobal(L, "runtime");

	return;
}

static void InitRuntimeForm(FormPtr form) {
	/* EMPTY */
	return;
}

static void DoneRuntimeForm(FormPtr form) {
	/* EMPTY */
	return;
}

/* runtime form event handling routine */

#warning "need to check more runtime routines ..."

static Boolean
frmRuntimeHandleEvent(EventPtr event) {
	Boolean handled;
	FormPtr form;
	UInt16 id;

	handled = false;
	form = FrmGetActiveForm();
	switch (event->eType) {
	case ctlSelectEvent:
		id = event->data.ctlSelect.controlID;
		switch (id) {
		case ID_BTN_RUNTIME_SOME:
			FrmCustomAlert(ID_FORM_INFORMATION, "some ...", "", "");
			runtime_open_func();
			handled = true;
			break;
		case ID_BTN_RUNTIME_CLOSE:
			/* NOT handled, have the default action close the form */
			break;
		default:
			/* EMPTY */
			break;
		}
		break;
	default:
		/* EMPTY */
		break;
	}
	return(handled);
}

/* application event handling routine */

#define HERE_IS_THE_APPLICATION
static Boolean
ApplicationHandleEvent(EventPtr event) {
	FormPtr form;
	UInt16 frmID;
	Boolean handled;

	handled = false;
	if (event->eType == frmLoadEvent) {
		frmID = event->data.frmLoad.formID;
		form = FrmInitForm(frmID);
		FrmSetActiveForm(form);

		switch (frmID) {
		case ID_FORM_MAIN:
			FrmSetEventHandler(form, frmMainHandleEvent);
			handled = true;
			break;
		case ID_FORM_RUNTIME:
			FrmSetEventHandler(form, frmRuntimeHandleEvent);
			handled = true;
			break;
		default:
			/* EMPTY */
			break;
		}
	}
	return(handled);
}

/* app's entry point, main event loop and special run modes */

UInt32
PilotMain(UInt16 cmd, void *cmdBPB, UInt16 launchFlags) {
	enum glueinit_retcode osrc;
	EventType event;
	Err error;
	UInt16 rc;
	UInt32 osver;
	MemHandle hdl;
	MemPtr ptr;
	char errtxt[16];

	/* XXX check ROM version? */
	/* scrollbars were introduced with PalmOS 2.0 */
	/* need PalmOS 3.5 for extended gadgets */
	/* might need PalmOS 4.x for l10n */
	/* app is using the File Manager (3.x?) */
	/* might reject anything below 2.x to avoid clumsy hacks */
	/* could check for an ARM processor later */
	/* could check for a color/mono display */
	/* could check for feature memory (above 64K) */

	/* run the application (only regular launch supported) */
	if (cmd == sysAppLaunchCmdNormalLaunch) {

		code_sections_reference();

		/* check PalmOS version */
		error = 0;
		FtrGet(sysFtrCreator, sysFtrNumROMVersion, &osver);
		if (osver < sysMakeROMVersion(3, 5, 0, sysROMStageRelease, 0)) {
			FrmCustomAlert(ID_FORM_FAILURE, "sorry, need PalmOS 3.5 or above", "", "");
			error = sysErrRomIncompatible;
		}
		if (error) {
			/* work around versions < 2.0 rerunning the app */
			if (osver < sysMakeROMVersion(2, 0, 0, sysROMStageRelease, 0)) {
				AppLaunchWithCommand(sysFileCDefaultApp, sysAppLaunchCmdNormalLaunch, NULL);
			}
			return(error);
		}

		/* make sure the runtime is available */
		osrc = osglue_init(CREATOR_CODE);
		error = 0;
		switch (osrc) {
		case GLUEINIT_OK:
			/* EMPTY */
			break;
		case GLUEINIT_LIB:
			error = sysErrLibNotFound;
			rc = FrmAlert(ID_FORM_MATHLIB_MISSING);
			if (rc == 1) {
				/* the user wants us to install the .prc */
				hdl = DmGetResource('Mprc', ID_DAT_MATHLIB_PRC);
				if (! hdl) {
					/* should never happen */
					FrmCustomAlert(ID_FORM_FAILURE, "oops?  cannot find my copy of the library", "", "");
					break;
				}
				ptr = MemHandleLock(hdl);
				error = DmCreateDatabaseFromImage(ptr);
				if (error != 0) {
					StrIToA(errtxt, error);
					FrmCustomAlert(ID_FORM_FAILURE, "could not install the library error code is", errtxt, "");
				}
				MemHandleUnlock(hdl);
				DmReleaseResource(hdl);
			}
			break;
		default:
			error = sysErrNotInitialized;
			FrmAlert(ID_FORM_INIT_FAILED);
			break;
		}
		if (error) {
			/* upon error -- clean up what was prepared and bail out */
			osglue_done();
			return(error);
		}

		/* the usual PalmOS app main event loop */
		error = StartApplication();
		if (error) {
			StopApplication();
			osglue_done();
			return(error);
		}
		do {
			EvtGetEvent(&event, evtWaitForever);
			(void)(SysHandleEvent(&event) ||
				MenuHandleEvent(0, &event, &error) ||
				ApplicationHandleEvent(&event) ||
				FrmDispatchEvent(&event));
		} while (event.eType != appStopEvent);
		StopApplication();

		/* shutdown the runtime */
		osglue_done();

	}

	return(0);
}

/* ----- E O F ----------------------------------------------- */
