#ifndef WBRO_RENDER
#define WBRO_RENDER

#include <cairo/cairo.h>
#include <pango/pango.h>
#include <pango/pangocairo.h>

#define ALPHA 0.8

#define TO_DOUBLE(num) (double)num
#define TO_RGB_FMT(num) TO_DOUBLE(num)/255.0
#define INV_RGB(num) 255.0-TO_DOUBLE(num)
#define TO_ALPHA(num) TO_DOUBLE(num)/100.0

struct appstate;

struct wb_render{
    cairo_surface_t * cai_srfc;
    cairo_t * cai_context;
    struct wb_style_main * m_style;
	struct appstate * appstate;
};

#endif
