/* ----- palmapp.h ------------------------------------------- */
/*
 * header file for the application code and its resource file
 */

/*
 * TODO
 * - identify and remove unused items
 * - have (parts of) the .h file generated by pilrc?
 */

#ifndef PALMAPP_H
#define PALMAPP_H

#define PROGRAM_VERSION "0.1"


/* ----- main form menu, main form ----- */
#define ID_MENU_MAIN	1000
	/* ----- File ----- */
	#define ID_MITEM_MAIN_FILE_NEW		1011
	#define ID_MITEM_MAIN_FILE_OPEN		1012
	#define ID_MITEM_MAIN_FILE_CLOSE	1013
	#define ID_MITEM_MAIN_FILE_RUN		1014
	#define ID_MITEM_MAIN_FILE_QUIT		1015
	/* ----- Setup ----- */
	#define ID_MITEM_MAIN_SETUP_CONFIG	1021
	/* ----- Help ----- */
	#define ID_MITEM_MAIN_HELP_HELP		1031
	#define ID_MITEM_MAIN_HELP_ABOUT	1032

#define ID_FORM_MAIN	1100
	#define ID_LBL_MAIN_HELP	1101
	#define ID_FLD_MAIN_SCRIPT	1102
	#define ID_FLD_MAIN_OUTPUT	1103
	#define ID_SCR_MAIN_OUTPUT	1104
	#define ID_BTN_MAIN_EXEC	1105
	#define ID_BTN_MAIN_BYTE	1106
	#define ID_BTN_MAIN_SOURCE	1107


/* ----- setup form ----- */
#define ID_FORM_SETUP	1200
	#define ID_LBL_SETUP_HELP			1201
	/* output config */
	#define ID_BTN_SETUP_OUTPUT_MARKUP_NONE		1211
	#define ID_BTN_SETUP_OUTPUT_MARKUP_MONO		1212
	#define ID_BTN_SETUP_OUTPUT_MARKUP_COLOR	1213
	#define ID_FLD_SETUP_MAXLINES			1214
	#define ID_FLD_SETUP_MAXBYTES			1215
	#define ID_CHK_SETUP_HIRES			1216
	/* diag/devel config */
	#define ID_CHK_SETUP_DIAG_CAPA			1281
	/* form buttons */
	#define ID_BTN_SETUP_APPLY			1291
	#define ID_BTN_SETUP_CANCEL			1292


/* ----- source form menu, source form ----- */
#define ID_MENU_SOURCE	1300
	/* ----- File ----- */
	#define ID_MITEM_SOURCE_FILE_STURTLE	1311
	#define ID_MITEM_SOURCE_FILE_SBITMAP	1312
	#define ID_MITEM_SOURCE_FILE_SHOUSE	1313
	#define ID_MITEM_SOURCE_FILE_LMEMO	1314
	#define ID_MITEM_SOURCE_FILE_SMEMO	1315
	/* ----- Help ----- */
	#define ID_MITEM_SOURCE_HELP_HELP	1321

#define ID_FORM_SOURCE	1400
	#define ID_LBL_SOURCE_HELP	1401
	#define ID_FLD_SOURCE_INPUT	1402
	#define ID_SCR_SOURCE_INPUT	1403
	#define ID_BTN_SOURCE_EXEC	1404
	#define ID_BTN_SOURCE_BYTE	1405
	#define ID_BTN_SOURCE_CLOSE	1406


/* ----- file selector form ----- */
#define ID_FORM_FILESEL	1500
	#define ID_LBL_FILESEL_HELP	1501
	#define ID_LST_FILESEL_FILES	1502
	#define ID_BTN_FILESEL_OK	1503
	#define ID_BTN_FILESEL_CANCEL	1504
	#define ID_BTN_FILESEL_MKNEW	1505


/* ----- runtime form ----- */
#define ID_FORM_RUNTIME	1600
	#define ID_BTN_RUNTIME_CLOSE	1601
	#define ID_LBL_RUNTIME_HELP	1602
	#define ID_BTN_RUNTIME_SOME	1603


/* ----- several alerts ----- */
#define ID_FORM_ABOUT		2100
#define ID_FORM_INIT_FAILED	2200
#define ID_FORM_INFORMATION	2300
#define ID_FORM_FAILURE		2301
#define ID_FORM_MATHLIB_MISSING	2400
	#define ID_LBL_MATH_HELP	2401
#define ID_FORM_STDIN	2500


/* ----- embedded resources ----- */
#define ID_DAT_MATHLIB_PRC		8000

#define ID_STR_SAMPLE_TURTLE		8011
#define ID_STR_SAMPLE_BITMAP		8012
#define ID_STR_SAMPLE_HOUSE		8013

#define ID_BMP_ABOUT_LOGO		8021


#endif /* PALMAPP_H */

/* ----- E O F ----------------------------------------------- */