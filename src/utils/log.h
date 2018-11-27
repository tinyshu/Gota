#ifndef __LOG_H__
#define __LOG_H__

/* debug level a*/
#define LEVEL_INFOR       0x01 
#define LEVEL_WARNING     0x02   
#define LEVEL_ERROR       0x04  
#define LEVEL_CRITICAL    0x08  

void init_log();
void LOG(char* file_name, int line, char*func, int level, char* fmt, ...);

#define LOGINFO(fmt, ...) LOG(__FILE__, __LINE__, __FUNCTION__, LEVEL_INFOR, fmt, ##__VA_ARGS__)
#define LOGWARNING(fmt, ...) LOG(__FILE__, __LINE__, __FUNCTION__, LEVEL_WARNING, fmt, ##__VA_ARGS__)
#define LOGERROR(fmt, ...) LOG(__FILE__, __LINE__, __FUNCTION__, LEVEL_ERROR, fmt, ##__VA_ARGS__)
#define LOGCRITICAL(fmt, ...) LOG(__FILE__, __LINE__, __FUNCTION__, LEVEL_CRITICAL, fmt, ##__VA_ARGS__)


#endif
