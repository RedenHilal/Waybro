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

struct config_dispatch {
	char field_name[32];
	union {
		const char * default_str;
		const char ** default_a_str;
		const int default_int;
		const int * default_a_int;
		const double default_float;
		const double * default_a_float;
		void * default_ptr;
	};
	int array_size;
	void ( *custom_parse)(void * start, const struct config_dispatch * dispatch,
					struct wb_config_setting * mod_set);
	int offset;
	int field_type;
	int field_modifier;
};

struct wb_style_main {
    int width;
    int height;

	int padding;
	int fill_color;

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

void
wb_style_parse_config(struct config_dispatch * dp, int length, void * start,
				struct wb_config_setting * set);

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
