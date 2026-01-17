#ifndef WBRO_MACRO
#define WBRO_MACRO

#include <stdint.h>
#include <stdio.h>

#define u64 uint64_t
#define u32 uint32_t
#define u16 uint16_t
#define u8 uint8_t

#define ON_ERR(trigger) {printf("ERR on: %s\n", trigger); perror(trigger); exit(1);}

#endif
