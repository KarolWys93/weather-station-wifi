#ifndef __DEBUG_H
#define __DEBUG_H

#include <logger.h>

#define DEBUG 1
#if DEBUG
	#define Debug(__info,...) Logger(LOG_DBG, "Debug: " __info,##__VA_ARGS__)
#else
	#define Debug(__info,...)  
#endif
#endif
