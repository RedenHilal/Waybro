#include <math.h>

#include "render.h"
#include "core.h"
#include "style.h"

#include "api-table.h"


const struct wb_render_api wb_render_api_table = {
	.draw_rect = wb_render_draw_rect,
	.expose_area = wb_render_expose_area,
	.erase_area = wb_render_erase_area,
	.draw_text = wb_render_draw_text,
	.draw_text_special = wb_render_draw_text_special
};

static void get_center(PangoLayout * layout, int * x, int * y,
						int width, int height, int dx, int dy)
{

    int lwidth, lheight;
    pango_layout_get_size(layout, &lwidth,&lheight);

    lwidth /= PANGO_SCALE;
    lheight /= PANGO_SCALE;

    *x = dx + (width - lwidth) / 2;
    *y = dy + (height - lheight) / 2;
}

void wb_render_draw_rect(struct wb_render * wrender, int x, int y, int width, int height)
{
	struct wb_style_main * m_sty = wrender->m_style;

	cairo_rectangle(wrender->cai_context, x, y, width, height);
	cairo_fill(wrender->cai_context);

}

void wb_render_expose_area(struct wb_render * wrender, int x, int y, int width, int height)
{
	struct appstate * appstate = wrender->appstate;
	
	wl_surface_attach(appstate->surface, appstate->buffer, 0, 0);
	wl_surface_damage(appstate->surface, x,  y, width, height);
	wl_surface_commit(appstate->surface);
}

void wb_render_erase_area(struct wb_render * wrender, int x, int y, int width, int height)
{		
    cairo_save(wrender->cai_context);
    cairo_set_operator(wrender->cai_context, CAIRO_OPERATOR_CLEAR);
    cairo_rectangle(wrender->cai_context, x, y, width, height);
    cairo_fill(wrender->cai_context);
    cairo_restore(wrender->cai_context);
}

void wb_render_draw_text(struct wb_render * wrender, int x, int y,
						const char * string)
{	
	cairo_set_source_rgba(wrender->cai_context, 1, 1, 1, ALPHA);
	cairo_move_to(wrender->cai_context, x, y);
	cairo_show_text(wrender->cai_context, string);
}

void wb_render_draw_text_special(struct wb_render * wrender, int x, int y,
							 const char * string)
{

    PangoLayout * layout = pango_cairo_create_layout(wrender->cai_context);
    
	pango_layout_set_text(layout, string, -1);
	pango_layout_set_font_description(layout,
					pango_font_description_from_string("Comic Neue 11"));

	cairo_set_source_rgba(wrender->cai_context, 1, 1, 1, ALPHA);
	cairo_move_to(wrender->cai_context, x, y);
    
	pango_cairo_show_layout(wrender->cai_context, layout);

    g_object_unref(layout);    
}

void wb_render_draw_arc(struct wb_render * wrender, struct wb_style_base * base,
						int width)
{
	cairo_t * cai_context =  wrender->cai_context;
	struct wb_style_main * m_sty = wrender->m_style;
   
    int lx = base->x  + base->rd_left;
    int rx = width + base->rd_left + base->x;
    int y = base->y + base->height/2;
   
    cairo_set_source_rgba(cai_context, TO_RGB_FMT(m_sty->r), TO_RGB_FMT(m_sty->g),
                        TO_RGB_FMT(m_sty->b), TO_ALPHA(m_sty->a));  
                    
    cairo_new_path(cai_context);
    cairo_move_to(cai_context, lx, base->y + base->height);

    if (base->rd_left)
        cairo_arc(cai_context, lx, y, base->rd_left, M_PI_2, 3 * M_PI_2);
    else
        cairo_line_to(cai_context, lx, base->y);

    cairo_line_to(cai_context, rx, base->y);

    if (base->rd_right)
        cairo_arc(cai_context, rx, y, base->rd_right, -M_PI_2, M_PI_2);
    else
        cairo_line_to(cai_context, rx, base->y + base->height);

    cairo_line_to(cai_context, lx,base->y + base->height);

    cairo_close_path(cai_context);
    cairo_fill(cai_context);
    
}
