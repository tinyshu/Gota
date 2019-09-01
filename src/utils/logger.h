#ifndef __LOGGER_H__
#define __LOGGER_H__

enum {
	LOG_DEBUG = 0,
	LOG_WARNING=1,
	LOG_ERROR=2,
};

#define log_debug(msg, ...) logger::log(__FILE__, __LINE__, LOG_DEBUG, msg, ## __VA_ARGS__);
#define log_warning(msg, ...) logger::log(__FILE__, __LINE__, LOG_WARNING, msg, ## __VA_ARGS__);
#define log_error(msg, ...) logger::log(__FILE__, __LINE__, LOG_ERROR, msg, ## __VA_ARGS__);

class logger {
public:
	static void init(const char* path,const char* prefix, bool std_output = false);
	static void log(const char* file_name, 
	                int line_num, 
	                int level, const char* msg, ...);
};


#endif

