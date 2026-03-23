#ifndef WBRO_WIDGET
#define WBRO_WIDGET

#include "clay.h"
#include "macro.h"

#define INT_TO_CLAY(rgba) GET_RED(rgba), GET_GREEN(rgba), GET_BLUE(rgba), GET_ALPHA(rgba)
#define WB_CLAY_RADIUS_ALL(radius) radius, radius, radius, radius

#define WB_WIDGET_INTEREST_SIZE 128

struct wb_context;
struct wb_render;

enum wb_widget_fnc_type {
	WB_WIDGET_RECT,
	WB_WIDGET_TEXT
};

enum wb_widget_sizing_type {
	WB_WIDGET_GROW,
	WB_WIDGET_FIXED,
	WB_WIDGET_PERCENT,
	WB_WIDGET_FIT
};

enum wb_widget_font {
	WB_WIDGET_FONT_PRIMARY,
	WB_WIDGET_FONT_SECONDARY,
	WB_WIDGET_FONT_ADD,
};

enum wb_widget_text_wrap {
	WB_WIDGET_WRAP_WORD,
	WB_WIDGET_WRAP_NEWLINE,
	WB_WIDGET_WRAP_NONE,
};

enum wb_widget_text_alignment {
	WB_WIDGET_ALIGN_LEFT,
	WB_WIDGET_ALIGN_CENTER,
	WB_WIDGET_ALIGN_RIGHT,
};

enum wb_widget_layout_direction {
	WB_WIDGET_LEFT_TO_RIGHT,
	WB_WIDGET_TOP_TO_BOTTOM
};

enum wb_widget_layout_alignment {
	WB_WIDGET_TOP,
	WB_WIDGET_LEFT = WB_WIDGET_TOP,
	WB_WIDGET_BOTTOM,
	WB_WIDGET_RIGHT = WB_WIDGET_BOTTOM,
	WB_WIDGET_CENTER
};

struct wb_widget_color {
	float r;
	float g;
	float b;
	float a;
};

struct wb_widget_padding {
	u16 left;
	u16 right;
	u16 top;
	u16 bottom;
};

union wb_widget_sizing_unit {
	//Percent sizing
	float percent;
	//Fixed sizing
	int size;
	//Grow and Fit sizing
	struct {
		int min;
		int max;
	};
};

enum wb_widget_special_event {
	WB_POINTER_ENTER			= 1 << 0,
	WB_POINTER_LEAVE			= 1 << 1,
	WB_POINTER_MOTION			= 1 << 2,
	WB_POINTER_BUTTON			= 1 << 3,
	WB_POINTER_AXIS				= 1 << 4,
	WB_POINTER_AXIS_SOURCE		= 1 << 5,
	WB_POINTER_AXIS_STOP		= 1 << 6,
	WB_POINTER_AXIS_DISCRETE	= 1 << 7
};

struct wb_widget_listen_node {
	char id[64];
	void * data;

	void (* on_enter)(void * data);
	void (* on_leave)(void * data);
	void (* on_click)(void * data);
	void (* on_scroll)(void * data);

	/* 
	 * event mask
	 */
	int events;
};

struct wb_widget_interest_list {
	struct wb_widget_listen_node node[WB_WIDGET_INTEREST_SIZE];
	int ncount;
};

struct wb_widget_rect_basic {
	struct wb_widget_color fill_color;
	struct wb_widget_color border_color;
	struct wb_widget_padding padding;
	int border_width;
	int radius;
	int child_gap;
	enum wb_widget_sizing_type sizing_width;
	enum wb_widget_sizing_type sizing_height;
	union wb_widget_sizing_unit width;
	union wb_widget_sizing_unit height;
	enum wb_widget_layout_alignment layout_x;
	enum wb_widget_layout_alignment layout_y;
	enum wb_widget_layout_direction direction;

	void (* child_cb)(void * data);
	void * data;
};

struct wb_widget_rect_special {
	struct wb_widget_rect_basic rect;

	struct wb_widget_listen_node event;
};

/*
 * unicode support with pango backend
 */
struct wb_widget_text_data {
	char * string;
	enum wb_widget_font font;
	int font_size;
	enum wb_widget_text_wrap wrap;
	enum wb_widget_text_alignment alignment;
	struct wb_widget_color text_color;
};

union wb_widget_render_data {
	struct wb_widget_rect_basic rect;
	struct wb_widget_text_data text;
};

struct wb_widget_child {
	int render_type;
	union wb_widget_render_data render_data;
};

void
wb_layout_arena_allocate(int width, int height);

void
wb_layout_context_init(struct wb_render * wrender);

void
wb_layout_font_init(char * fonts[]);

void
wb_layout_begin();

void
wb_layout_compute(struct wb_context * ctx);

void
wb_widget_region_clean(struct wb_context * ctx);

void
wb_widget_rect(struct wb_widget_rect_basic * data);

void
wb_widget_rect_special(struct wb_widget_rect_special * data, struct wb_context * ctx);

void
wb_widget_text(struct wb_widget_text_data * data);

#endif
