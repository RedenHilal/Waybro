#ifndef WSTYLE_H
#define WSTYLE_H

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

struct component_style{
    char key[32];
    union {
        char str_val[64];
        int int_val;
        double db_val; 
    };
    enum style_type type;
    enum style_unit unit;
    UT_hash_handle hh;
};

struct component_entries{
    char section_key[32];
    struct component_style * style;
    UT_hash_handle hh;
};

#define M_STYLE_COUNT 6

struct m_style {
    // properties count is defined
    // please consider to update it if changes made
    int width;
    int height;
    uint8_t a;
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

struct base_style {
    int width;
    int height;
    int x;
    int y;
    int font_size;
    char enabled;
    char rd_left;
    char rd_right;
};

struct ws_style {
    struct base_style base;
    int max_ws;
    int radius;
    int h_color;
};

struct tm_style {
    struct base_style base;
    char format[64];
};

struct brightness_style {
    struct base_style base;
    char format[64];
};

struct vol_style {
    struct base_style base;
    char format[64];
};

struct blue_style {
    struct base_style base;
    char format[64];
};

struct net_style {
    struct base_style base;
    char format[64];
};

struct power_style {
    struct base_style base;
    char format[64];
};

struct mpd_style {
    struct base_style base;
    char format[64];
};

struct mem_style {
    struct base_style base;
    char format[64];
    int it_sec;
};

struct temp_style {
    struct base_style base;
    char format[64];
    int it_sec;
};


void * get_sys_sty(struct component_entries ** entries, struct m_style * m_style);
void * get_ws_sty(struct component_entries ** entries, struct m_style * m_style);
void * get_tm_sty(struct component_entries ** entries, struct m_style * m_style);
void * get_brght_sty(struct component_entries ** entries, struct m_style * main_sty);
void * get_vol_sty(struct component_entries ** entries, struct m_style * main_sty);
void * get_blue_sty(struct component_entries ** entries, struct m_style * m_style);
void * get_net_sty(struct component_entries ** entries, struct m_style * m_style);
void * get_power_sty(struct component_entries ** entries, struct m_style * m_style);
void * get_mpd_sty(struct component_entries ** entries, struct m_style * m_style);
void * get_mem_sty(struct component_entries ** entries, struct m_style * m_style);
void * get_temp_sty(struct component_entries ** entries, struct m_style * m_style);


#endif
