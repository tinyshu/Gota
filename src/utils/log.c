#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "log.h"

#ifndef WIN32
#include <sys/time.h>
#endif

#include <stdio.h>  
#include <time.h>  

/* debug level define */
int g_dbg_level = LEVEL_INFOR | LEVEL_WARNING | LEVEL_ERROR | LEVEL_CRITICAL;
FILE *g_log_fp = NULL;

// __FUNCTION__, __FILE__, __LINE__
void 
LOG(char* file_name, int line, char* func, int level, char* fmt, ...)
{ 
	char buf[256];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, 256, fmt, args);
	va_end(args);

	time_t t = time(NULL);
	struct tm *tmm = localtime(&t);
	if (g_dbg_level & level) {
		printf("[%d-%02d-%02d %02d:%02d:%02d][%s][%s:%d]:%s\n", 
			tmm->tm_year + 1900, tmm->tm_mon + 1, tmm->tm_mday, 
			tmm->tm_hour, tmm->tm_min, tmm->tm_sec, 
			func, file_name, line, buf);
	}
		
	
	if ((g_log_fp != NULL) && (level >= LEVEL_CRITICAL)) {
		fprintf(g_log_fp, "[%d-%02d-%02d %02d:%02d:%02d][%s][%s:%d]:%s\n", 
			tmm->tm_year + 1900, tmm->tm_mon + 1, tmm->tm_mday, 
			tmm->tm_hour, tmm->tm_min, tmm->tm_sec,
			func, file_name, line, buf);
	}
		
}

void 
init_log() {

}
