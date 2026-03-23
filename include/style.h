#ifndef WSTYLE_H
#define WSTYLE_H

#include "macro.h"

#define WB_PADDING_ALL(pd) {pd, pd, pd, pd}
#define WB_COLOR_FROM_RGBA(color) GET_RED(color), GET_GREEN(color), GET_BLUE(color),\
		GET_ALPHA(color)

#define WB_STYLE_STR_SIZE 64

struct wb_config;
struct wb_config_setting;

extern unsigned int
wb_style_type_size[];

enum wb_style_group {
	WB_STYLE_GROUP_LEFT,
	WB_STYLE_GROUP_CENTER,
	WB_STYLE_GROUP_RIGHT
};

enum wb_style_field_type {
	WB_STYLE_INT,
	WB_STYLE_LL,
	WB_STYLE_BOOL,
	WB_STYLE_FLOAT,
	WB_STYLE_STRING,
	WB_STYLE_CUSTOM
};

enum wb_style_field_modifier {
	WB_STYLE_ARRAY = 1,
	WB_STYLE_LIST
};

struct wb_style_main {
    int width;
    int height;

	int padding;
	int fill_color;

	/*
	 * allocated into
	 * char fonts[n + 1][WB_STYLE_STR_SIZE]
	 * with last element fonts[n + 1][] as null bound
	 */
	char ** fonts;
	int font_size;

	int text_color;
	int radius;

	int border_width;
	int border_color;

	int group_color;
	int group_radius;
	int group_border_width;
	int group_border_color;

	int module_radius;
	int module_border_width;
	int module_border_color;
	int module_color;
	int module_color_hover;
};



struct wb_style_main *
wb_style_get_main(struct wb_config_setting * wcfg);



// unused

struct wb_style_base {
	int width;
	int margin_left;
	int margin_right;
};


void
wb_style_get_base(struct wb_style_base * base, struct wb_config_setting * mod_set,
				struct wb_style_main * msty);

#endif
