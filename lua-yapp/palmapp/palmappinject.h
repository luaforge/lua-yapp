/* ----- palmappinject.h ------------------------------------- */
/*
 * interface to the PalmOS application for the C runtime library
 */

#ifndef PALMAPPINJECT_H
#define PALMAPPINJECT_H

enum app_event_code {
	APP_EVENT_EXIT,
	APP_EVENT_STDIN,
	APP_EVENT_STDOUT,
	APP_EVENT_STDERR,
};

union app_event_desc {
	struct {
		int retcode;
	} exit;
	struct {
		char *buffer;
		int length;
		int delimiter;
		int read_count;
		int had_error;
		int had_eof;
	} stdio;
};

void app_add_event(const enum app_event_code code, union app_event_desc *data) CSEC_APP;
void app_run_event_handler(const int bWait) CSEC_APP;

void app_add_event(const enum app_event_code code, union app_event_desc *data);
void app_run_event_handler(const int bWait);

#endif /* PALMAPPINJECT_H */

/* ----- E O F ----------------------------------------------- */
