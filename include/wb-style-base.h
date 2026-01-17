#ifndef WBRO_STYLE_BASE
#define WBRO_STYLE_BASE

/* 
 * basic style parsing utility
 */

#define WB_STYLE_STR_SIZE_MAX 64

struct wb_style_base {
    int width;
    int height;
    int x;
    int y;
    int font_size;
    char enabled;
    char rd_left;
    char rd_right;
};

#endif
