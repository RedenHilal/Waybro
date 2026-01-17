#ifndef WSTYLE_H
#define WSTYLE_H

#include "wb-style-base.h"
#include "uthash.h"

#define GET_RED(color) (double)((color & 0xff000000) >> 24) / 255
#define GET_GREEN(color) (double)((color & 0xff0000) >> 16) / 255
#define GET_BLUE(color) (double)((color & 0xff00) >> 8) / 255

enum {
    ARC_LEFT,
    ARC_RIGHT
};

enum style_unit {
    NONE,
    PCTG,
    PX
};

enum style_type {
    INT,
    STRING,
    DOUBLE
};

struct wb_style_unit{
    char key[32];
    union {
        char str_val[WB_STYLE_STR_SIZE_MAX];
        int int_val;
        double db_val; 
    };
    enum style_type type;
    enum style_unit unit;
    UT_hash_handle hh;
};

struct wb_style_sec{
    char section_key[32];
    struct wb_style_unit * style;
    UT_hash_handle hh;
};

#define M_STYLE_COUNT 6

struct wb_style_main {
    // properties count is defined
    // please consider to update it if changes made
    int width;
    int height;
    uint8_t a;
    uint8_t r;
    uint8_t g;
    uint8_t b;
	char font[64];
};



struct tm_style {
    struct wb_style_base base;
    char format[64];
};

struct brightness_style {
    struct wb_style_base base;
    char format[64];
};

struct vol_style {
    struct wb_style_base base;
    char format[64];
};

struct blue_style {
    struct wb_style_base base;
    char format[64];
};

struct net_style {
    struct wb_style_base base;
    char format[64];
};


struct mpd_style {
    struct wb_style_base base;
    char format[64];
};

struct mem_style {
    struct wb_style_base base;
    char format[64];
    int it_sec;
};

struct temp_style {
    struct wb_style_base base;
    char format[64];
    int it_sec;
};



// config parsing
int read_config(char * path, struct wb_style_sec ** cpn_entries);
struct wb_style_main * translate_mstyle(struct wb_style_sec **);
void get_base_sty(struct wb_style_base * base, struct wb_style_sec * sec, 
				struct wb_style_main * mtsy);

#endif
