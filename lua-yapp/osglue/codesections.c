/* ----- codesections.c -------------------------------------- */
/*
 * since the linker insists in all segments declared in the .def
 * file to be referenced by the binary (contain _some_ code or
 * data), we will put at least one dummy routine into each
 * segment
 *
 * this allows for easy rearrangement of code without the need to
 * adjust the .def file
 */

static void csref_lua(void) __attribute__((section("cs_lua")));
static void csref_lapi(void) __attribute__((section("cs_lapi")));
static void csref_lauxlib(void) __attribute__((section("cs_lauxl")));
static void csref_lbaslib(void) __attribute__((section("cs_lbasl")));
static void csref_lcode(void) __attribute__((section("cs_lcode")));
static void csref_ldblib(void) __attribute__((section("cs_ldblb")));
static void csref_ldebug(void) __attribute__((section("cs_ldbg")));
static void csref_ldo(void) __attribute__((section("cs_ldo")));
static void csref_ldump(void) __attribute__((section("cs_ldump")));
static void csref_lfunc(void) __attribute__((section("cs_lfunc")));
static void csref_lgc(void) __attribute__((section("cs_lgc")));
static void csref_linit(void) __attribute__((section("cs_linit")));
static void csref_liolib(void) __attribute__((section("cs_liolb")));
static void csref_llex(void) __attribute__((section("cs_llex")));
static void csref_lmathlib(void) __attribute__((section("cs_lmath")));
static void csref_lmem(void) __attribute__((section("cs_lmem")));
static void csref_loadlib(void) __attribute__((section("cs_ldlib")));
static void csref_lobject(void) __attribute__((section("cs_lobj")));
static void csref_lopcodes(void) __attribute__((section("cs_lcode")));
static void csref_loslib(void) __attribute__((section("cs_loslb")));
static void csref_lparser(void) __attribute__((section("cs_lprsr")));
static void csref_lstate(void) __attribute__((section("cs_lstat")));
static void csref_lstring(void) __attribute__((section("cs_lstr")));
static void csref_lstrlib(void) __attribute__((section("cs_lstrl")));
static void csref_ltable(void) __attribute__((section("cs_ltbl")));
static void csref_ltablib(void) __attribute__((section("cs_ltblb")));
static void csref_ltm(void) __attribute__((section("cs_ltm")));
static void csref_luac(void) __attribute__((section("cs_luac")));
static void csref_lunump(void) __attribute__((section("cs_ldump")));
static void csref_lvm(void) __attribute__((section("cs_lvm")));
static void csref_lzio(void) __attribute__((section("cs_lzio")));
static void csref_lprint(void) __attribute__((section("cs_lprnt")));
static void csref_rtl(void) __attribute__((section("cs_rtl")));
static void csref_mathrtl(void) __attribute__((section("cs_martl")));
static void csref_app(void) __attribute__((section("cs_app")));
static void csref_plib(void) __attribute__((section("cs_plib")));

static void csref_lua(void) { /* EMPTY */ }
static void csref_lapi(void) { /* EMPTY */ }
static void csref_lauxlib(void) { /* EMPTY */ }
static void csref_lbaslib(void) { /* EMPTY */ }
static void csref_lcode(void) { /* EMPTY */ }
static void csref_ldblib(void) { /* EMPTY */ }
static void csref_ldebug(void) { /* EMPTY */ }
static void csref_ldo(void) { /* EMPTY */ }
static void csref_ldump(void) { /* EMPTY */ }
static void csref_lfunc(void) { /* EMPTY */ }
static void csref_lgc(void) { /* EMPTY */ }
static void csref_linit(void) { /* EMPTY */ }
static void csref_liolib(void) { /* EMPTY */ }
static void csref_llex(void) { /* EMPTY */ }
static void csref_lmathlib(void) { /* EMPTY */ }
static void csref_lmem(void) { /* EMPTY */ }
static void csref_loadlib(void) { /* EMPTY */ }
static void csref_lobject(void) { /* EMPTY */ }
static void csref_lopcodes(void) { /* EMPTY */ }
static void csref_loslib(void) { /* EMPTY */ }
static void csref_lparser(void) { /* EMPTY */ }
static void csref_lstate(void) { /* EMPTY */ }
static void csref_lstring(void) { /* EMPTY */ }
static void csref_lstrlib(void) { /* EMPTY */ }
static void csref_ltable(void) { /* EMPTY */ }
static void csref_ltablib(void) { /* EMPTY */ }
static void csref_ltm(void) { /* EMPTY */ }
static void csref_luac(void) { /* EMPTY */ }
static void csref_lunump(void) { /* EMPTY */ }
static void csref_lvm(void) { /* EMPTY */ }
static void csref_lzio(void) { /* EMPTY */ }
static void csref_lprint(void) { /* EMPTY */ }
static void csref_rtl(void) { /* EMPTY */ }
static void csref_mathrtl(void) { /* EMPTY */ }
static void csref_app(void) { /* EMPTY */ }
static void csref_plib(void) { /* EMPTY */ }

void code_sections_reference(void) {
	csref_lua();
	csref_lapi();
	csref_lauxlib();
	csref_lbaslib();
	csref_lcode();
	csref_ldblib();
	csref_ldebug();
	csref_ldo();
	csref_ldump();
	csref_lfunc();
	csref_lgc();
	csref_linit();
	csref_liolib();
	csref_llex();
	csref_lmathlib();
	csref_lmem();
	csref_loadlib();
	csref_lobject();
	csref_lopcodes();
	csref_loslib();
	csref_lparser();
	csref_lstate();
	csref_lstring();
	csref_lstrlib();
	csref_ltable();
	csref_ltablib();
	csref_ltm();
	csref_luac();
	csref_lunump();
	csref_lvm();
	csref_lzio();
	csref_lprint();
	csref_rtl();
	csref_mathrtl();
	csref_app();
	csref_plib();
}

/* ----- E O F ----------------------------------------------- */
