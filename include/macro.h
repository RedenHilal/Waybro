#ifndef WBRO_MACRO
#define WBRO_MACRO

#include <stdint.h>
#include <stdio.h>

#define u64 uint64_t
#define u32 uint32_t
#define u16 uint16_t
#define u8 uint8_t

#define ON_ERR(trigger) {printf("ERR on: %s\n", trigger); perror(trigger); exit(1);}

#define GET_RED(color)		(double)((color >> 24) & 0xff) 
#define GET_GREEN(color)	(double)((color >> 16) & 0xff) 
#define GET_BLUE(color)		(double)((color >> 8) & 0xff) 
#define GET_ALPHA(color)	(double)(color & 0xff) 

#define WB_COLOR_FROM_RGBA(color) {GET_RED(color), GET_GREEN(color), GET_BLUE(color),\
		GET_ALPHA(color)}

#define WB_LOG_CRIT 1 << 0
#define WB_LOG_ERR 1 << 1
#define WB_LOG_WARN 1 << 2
#define WB_LOG_INFO 1 << 3

#ifndef WB_LOG_LEVEL
#define WB_LOG_LEVEL 0b1111
#endif

#if WB_LOG_LEVEL & WB_LOG_CRIT

#define LOG_CRIT(...) \
		fprintf(stderr, "CRITICAL: "); \
		fprintf(stderr, __VA_ARGS__); 
#else

#define LOG_CRIT(...) ((void)0)

#endif


#if WB_LOG_LEVEL & WB_LOG_ERR

#define LOG_ERR(...) \
		fprintf(stderr, "ERROR: "); \
		fprintf(stderr, __VA_ARGS__);

#else

#define LOG_ERR(...) ((void)0)

#endif


#if WB_LOG_LEVEL & WB_LOG_WARN

#define LOG_WARN(...) \
		fprintf(stderr, "WARNING: "); \
		fprintf(stderr, __VA_ARGS__); 

#else

#define LOG_WARN(...) ((void)0)

#endif


#if WB_LOG_LEVEL & WB_LOG_INFO
#define LOG_INFO(...) \
		fprintf(stderr, "INFO: "); \
		fprintf(stderr, __VA_ARGS__);

#else

#define LOG_INFO(...) ((void)0)

#endif

#endif
