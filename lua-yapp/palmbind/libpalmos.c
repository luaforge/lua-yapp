/* ----- libpalmos.c ----------------------------------------- */

/* tiny PalmOS binding for Lua */

#include <PalmOS.h>

#include "libpalmos.h"
#include <lualib.h>
#include <lauxlib.h>

#include <string.h>
#include <math.h>

/*
 * global state
 * and initialization
 */


static int palmos_init(lua_State *L) CSEC_PLIB;
static int palmos_alert(lua_State *L) CSEC_PLIB;
static int palmos_openres(lua_State *L) CSEC_PLIB;
static int palmos_closeres(lua_State *L) CSEC_PLIB;
static int palmos_getsize(lua_State *L) CSEC_PLIB;
static int palmos_getres(lua_State *L) CSEC_PLIB;
static int palmos_mode(lua_State *L) CSEC_PLIB;
static int palmos_rgb(lua_State *L) CSEC_PLIB;
static int palmos_clear(lua_State *L) CSEC_PLIB;
static int palmos_color(lua_State *L) CSEC_PLIB;
static int palmos_moveto(lua_State *L) CSEC_PLIB;
static int palmos_pos(lua_State *L) CSEC_PLIB;
static int palmos_box(lua_State *L) CSEC_PLIB;
static int palmos_drawbmp(lua_State *L) CSEC_PLIB;
static int palmos_heading(lua_State *L) CSEC_PLIB;
static int palmos_walk(lua_State *L) CSEC_PLIB;
static int palmos_turn(lua_State *L) CSEC_PLIB;
static int palmos_event(lua_State *L) CSEC_PLIB;

int luaopen_palmos(lua_State *L);


/* the bounds we're allowed to move within */
static Coord xmin = 0;
static Coord xmax = 160;
static Coord ymin = 0;
static Coord ymax = 160;

/* current draw status */
static IndexedColorType bg;
static IndexedColorType fg;
static Coord x;
static Coord y;
static UInt16 hd;

/* init routine */
static int palmos_init(lua_State *L) {
	RectangleType rect;
	RGBColorType rgb;
	IndexedColorType idx;

	/*
	 * XXX TODO
	 *
	 * where to get the coordinates from?
	 * assume fullscreen? (currently done)
	 * tell full from half screen? i.e.
	 *   scripted from interactive mode? how?
	 * get a form's/gadget's/window's bounds?
	 * register a routine with Lua to
	 *   have the app tell us all this stuff?
	 */

	/* restrict ourselves to these bounds */
	memset(&rect, 0, sizeof(rect));
	rect.topLeft.x = xmin;
	rect.topLeft.y = xmin;
	rect.extent.x = xmax - xmin;
	rect.extent.y = ymax - ymin;
	WinSetClip(&rect);

	/* setup colors */
	memset(&rgb, 0, sizeof(rgb));

	/* black background */
	rgb.r = 0; rgb.g = 0; rgb.b = 0;
	idx = WinRGBToIndex(&rgb);
	bg = idx;

	/* white foreground */
	rgb.r = 255; rgb.g = 255; rgb.b = 255;
	idx = WinRGBToIndex(&rgb);
	fg = idx;

	/* setup start position */
	x = xmin;
	y = ymin;
	hd =0;

	return(0);
}

/*
 * alert(id)
 *
 * pops up the alert form with the specified id
 */

static int palmos_alert(lua_State *L) {
	UInt16 id;

	id = luaL_checkinteger(L, 1);
	FrmAlert(id);
	return(0);
}

/*
 * openres("type", id [, file])
 *
 * opens the resource, returns a number identifying the resource
 */
static int palmos_openres(lua_State *L) {
	UInt32 type;
	UInt16 id;
	MemHandle hdl;
	UInt32 hnum;

	if ((lua_gettop(L) < 2) || (lua_gettop(L) > 3)) {
		luaL_error(L, "wrong parameter count");
	}
	if (lua_gettop(L) == 3) {
		luaL_error(L, "DmGet1Resource() not supported ATM");
	}
	if (lua_isstring(L, 1)) {
		const char *s;
		int idx;
		s = luaL_checkstring(L, 1);
		type = 0;
		for (idx = 0; idx < sizeof(type); idx++) {
			/* yes, we don't insist in exactly four chars */
			if (s[idx] == '\0')
				break;
			type <<= 8;
			type |= s[idx];
		}
	} else if (lua_isnumber(L, 1)) {
		type = luaL_checknumber(L, 1);
	}
	id = luaL_checknumber(L, 2);

	hdl = DmGetResource(type, id);
	hnum= (UInt32)hdl;
	lua_pushnumber(L, hnum);
	return(1);
}

/*
 * closeres(id)
 *
 * closes the resource identified by "id" which was returned by
 * openres()
 */
static int palmos_closeres(lua_State *L) {
	UInt32 hnum;
	MemHandle hdl;

	hnum = luaL_checknumber(L, 1);
	hdl = (MemHandle)hnum;
	DmReleaseResource(hdl);

	return(0);
}

/*
 * getsize(id)
 *
 * returns the size of a resource, plus width and height if the resource
 * is a bitmap
 */
/* XXX TODO */
static int palmos_getsize(lua_State *L) {
	UInt32 hnum;
	MemHandle hdl;

	hnum = luaL_checknumber(L, 1);
	hdl = (MemHandle)hnum;
	lua_pushinteger(L, MemHandleSize(hdl));
	/*
	 * how to check for "is a bitmap"?
	 * BmpGetVersion()
	 * BmpGetDimensions()
	 * DmSearchRecord()?
	 */
	return(1);
}

/*
 * getres(id [, start [, end]])
 *
 * returns a (sub)string with the resource identified by "id"
 * (which was returned from openres()
 */
static int palmos_getres(lua_State *L) {
	UInt32 hnum;
	UInt32 start, end;
	MemHandle hdl;
	UInt8 *ptr;

	hnum = luaL_checknumber(L, 1);
	hdl = (MemHandle)hnum;
	if (lua_gettop(L) > 1)
		start = luaL_checknumber(L, 2);
	else
		start = 0;
	if (lua_gettop(L) > 2)
		end = luaL_checknumber(L, 3);
	else
		end = 0;

	ptr = MemHandleLock(hdl);
	if (ptr == NULL)
		luaL_error(L, "invalid resource id");
	if (start != 0)
		start -= 1; /* zero based */
	if (end != 0)
		end -= 1;
	else
		end = MemHandleSize(hdl);

	lua_pushlstring(L, ptr + start, end - start);
	return(1);
}

/*
 * mode()
 *
 * returns screen width, screen height, screen depth and color support
 */
static int palmos_mode(lua_State *L) {
	Err err;
	UInt32 w, h, d;
	Boolean c;

	err = WinScreenMode(winScreenModeGet, &w, &h, &d, &c);
	if (err)
		luaL_error(L, "cannot determine screen mode");

	lua_pushnumber(L, w);
	lua_pushnumber(L, h);
	lua_pushnumber(L, d);
	lua_pushboolean(L, (c) ? 1 : 0);
	return(4);
}

/*
 * rgb(r, g, b)
 *
 * returns the color (index?) equivalent to the R, G, B components
 */
static int palmos_rgb(lua_State *L) {
	RGBColorType rgb;
	IndexedColorType idx;

	memset(&rgb, 0, sizeof(rgb));
	rgb.r = luaL_checknumber(L, 1);
	rgb.g = luaL_checknumber(L, 2);
	rgb.b = luaL_checknumber(L, 3);
	idx = WinRGBToIndex(&rgb);
	lua_pushnumber(L, idx);
	return(1);
}

/*
 * clear([c])
 *
 * erases the screen with the specified color or with the background
 * color, moves the cursor to the upper lefthand corner
 */
static int palmos_clear(lua_State *L) {
	IndexedColorType idx, save;
	RectangleType rect;

	if (lua_gettop(L) > 0)
		idx = luaL_checknumber(L, 1);
	else
		idx = bg;
	save = WinSetForeColor(idx);
	memset(&rect, 0, sizeof(rect));
	rect.topLeft.x = xmin;
	rect.topLeft.y = ymin;
	rect.extent.x = xmax - xmin;
	rect.extent.y = ymax - ymin;
	WinFillRectangle(&rect, 0);
	WinSetForeColor(save);
	return(0);
}

/*
 * color(fg, [bg])
 *
 * sets the foreground and optionally the background color
 */
static int palmos_color(lua_State *L) {
	IndexedColorType idx;

	idx = luaL_checknumber(L, 1);
	WinSetForeColor(idx);
	WinSetTextColor(idx);

	if (lua_gettop(L) > 1) {
		idx = luaL_checknumber(L, 2);
		WinSetBackColor(idx);
	}

	return(0);
}

/*
 * moveto(x, y)
 *
 * moves the cursor to the specified position
 */
static int palmos_moveto(lua_State *L) {
	Coord mx, my;

	mx = luaL_checknumber(L, 1);
	my = luaL_checknumber(L, 2);

	/* apply the new position (relative to our bounds) */
	x = mx + xmin;
	y = my + ymin;

	/* restrict the position to within the bounds */
	if (x < xmin)
		x = xmin;
	if (x >= xmax)
		x = xmax - 1;
	if (y < ymin)
		y = ymin;
	if (y >= ymax)
		y = ymax - 1;

	return(0);
}

/*
 * pos()
 *
 * returns the current x and y cursor position
 */
static int palmos_pos(lua_State *L) {
	lua_pushnumber(L, x - xmin);
	lua_pushnumber(L, y - ymin);
	return(2);
}

/*
 * box(x, y, w, h, [c])
 *
 * draws a filled rectangle from point (x, y), extending w and h pixels,
 * in the specified color or the current bg color
 */
static int palmos_box(lua_State *L) {
	RectangleType rect;
	IndexedColorType idx, save;

	memset(&rect, 0, sizeof(rect));
	rect.topLeft.x = luaL_checknumber(L, 1);
	rect.topLeft.y = luaL_checknumber(L, 2);
	rect.extent.x = luaL_checknumber(L, 3);
	rect.extent.y = luaL_checknumber(L, 4);
	idx = bg;
	if (lua_gettop(L) > 4)
		idx = luaL_checknumber(L, 5);

	save = WinSetForeColor(idx);
	WinFillRectangle(&rect, 0);
	WinSetForeColor(save);

	return(0);
}

/*
 * drawbmp(id)
 *
 * draws the bitmap identified by "id" which was returned from
 * openres(), advance the cursor to the right of the bitmap
 */
static int palmos_drawbmp(lua_State *L) {
	UInt32 hnum;
	MemHandle hdl;
	BitmapType *bmp;

	hnum = luaL_checknumber(L, 1);
	hdl = (MemHandle)hnum;
	bmp = MemHandleLock(hdl);
	if (bmp == NULL)
		luaL_error(L, "invalid bitmap id");
	WinDrawBitmap(bmp, x, y);
	MemHandleUnlock(hdl);
	return(0);
}

/*
 * heading(g)
 *
 * turns the turtle into the specified direction in degrees, 0 is right,
 * 90 is up, 180 is left, 270 is down
 */
static int palmos_heading(lua_State *L) {
	typeof(hd) deg;

	deg = luaL_checknumber(L, 1);
	hd = deg;

	return(0);
}

/*
 * walk(d)
 *
 * draws a line from the current position following the current heading
 * with extent d, position the cursor at the line's end
 */
static int palmos_walk(lua_State *L) {
	Coord dx, dy;
	Coord dist;
	double dbl;

#ifndef MATH_PI
  #define MATH_PI 3.141592635
#endif

	/* get the distance */
	dist = luaL_checknumber(L, 1);

	/* determine the x motion */
	dbl = hd;
	dbl /= 180.;
	dbl *= MATH_PI;
	dbl = cos(dbl);
	dbl *= dist;
	dx = dbl;

	/* determine the y motion */
	dbl = hd;
	dbl /= 180.;
	dbl *= MATH_PI;
	dbl = sin(dbl);
	dbl *= dist;
	dy = -dbl; /* positive sin() result decreases y, we're going north */

	/* draw the line, advance the cursor */
	WinDrawLine(x, y, x + dx, y + dy);
	x += dx; y += dy;

	return(0);
}

/*
 * turn()
 *
 * returns the current x and y cursor position
 */
static int palmos_turn(lua_State *L) {
	lua_pushnumber(L, x - xmin);
	lua_pushnumber(L, y - ymin);
	return(2);
}

/*
 * event()
 *
 * returns the current x and y cursor position
 */
static int palmos_event(lua_State *L) {
	lua_pushnumber(L, x - xmin);
	lua_pushnumber(L, y - ymin);
	return(2);
}

#warning "need to finish PalmOS binding"

static const luaL_Reg palmoslib[] = {
	/* UI */
	{ "alert", palmos_alert, },
	/* Database */
	{ "openres", palmos_openres, },
	{ "closeres", palmos_closeres, },
	{ "getsize", palmos_getsize, },
	{ "getres", palmos_getres, },
	/* drawing */
	{ "mode", palmos_mode, },
	{ "rgb", palmos_rgb, },
	{ "clear", palmos_clear, },
	{ "color", palmos_color, },
	{ "moveto", palmos_moveto, },
	{ "pos", palmos_pos, },
	{ "box", palmos_box, },
	{ "drawbmp", palmos_drawbmp, },
	/* logo stuff */
	{ "heading", palmos_heading, },
	{ "walk", palmos_walk, },
	{ "turn", palmos_turn, },
	/* event reading */
	{ "event", palmos_event, },
	/* end of list */
	{ NULL, NULL, },
};

int luaopen_palmos(lua_State *L) {
	luaL_register(L, "palmos", palmoslib);
	palmos_init(L);
	return(1);
}

/* ----- E O F ----------------------------------------------- */
