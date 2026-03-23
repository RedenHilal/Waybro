#include <cairo/cairo.h>
#include <stdio.h>
#include <stdlib.h>

#include "../../include/widget.h"

#define WIDTH  640
#define HEIGHT 420
#define RGBA(r,g,b,a) (struct wb_widget_color){ (r), (g), (b), (a) }

int
main(void)
{
    cairo_surface_t *surface =
        cairo_image_surface_create(CAIRO_FORMAT_ARGB32, WIDTH, HEIGHT);
    cairo_t *c = cairo_create(surface);

	cairo_set_source_rgba(c, 0.95, 0.95, 0.95, 1);
    cairo_paint(c);

    wb_layout_arena_allocate(WIDTH, HEIGHT);
    wb_layout_context_init(c);

    char *fonts[] = {
        "Quicksand Semibold",
        "Callistoga",
        NULL
    };

    wb_layout_font_init(fonts);

    wb_layout_begin_layout();



	struct wb_widget_rect_data child1 = {
	    .fill_color   = RGBA(9, 6, 6, 255),
	    .border_color = RGBA(8, 3, 3, 255),
	    .border_width = 2,
	    .radius       = 8,
	    .child_gap    = 0,
	
	    .sizing_width  = WB_WIDGET_FIXED,
	    .sizing_height = WB_WIDGET_FIXED,
	    .width.size    = 120,
	    .height.size   = 60,
	
	    .layout_x = WB_WIDGET_CENTER,
	    .layout_y = WB_WIDGET_CENTER,
	};
	
	struct wb_widget_rect_data child2 = {
	    .fill_color   = RGBA(60, 75, 9, 255),
	    .border_color = RGBA(30, 45, 80, 255),
	    .border_width = 2,
	    .radius       = 8,
	    .child_gap    = 0,
	
	    .sizing_width  = WB_WIDGET_GROW,
	    .sizing_height = WB_WIDGET_FIXED,
	    .width.min     = 80,
	    .width.max     = 300,
	    .height.size   = 60,
	
	    .layout_x = WB_WIDGET_CENTER,
	    .layout_y = WB_WIDGET_CENTER,
	};
	
	
	

	struct wb_widget_text_data ts_data = {
		.string = "jfasdijasffashui",
		.font_size = 12,
		.text_color = RGBA(20,20,1,255)
	};

	struct wb_widget_child container_children[] = {
	    {
	        .render_type = WB_WIDGET_RECT,
	        .render_data.rect = child1,
	    },
	    {
	        .render_type = WB_WIDGET_RECT,
	        .render_data.rect = child2,
	    },
		{
			.render_type = WB_WIDGET_TEXT,
			.render_data.text = ts_data,
		}
	};
	
	struct wb_widget_rect_data root = {
	    .fill_color   = RGBA(85, 8, 20, 255),
	    .border_color = RGBA(2, 20, 25, 155),
	    .border_width = 3,
	    .radius       = 12,
	    .child_gap    = 0,
	
	    .sizing_width  = WB_WIDGET_PERCENT,
	    .sizing_height = WB_WIDGET_PERCENT,
	    .width.percent  = 0.9,
	    .height.percent = 0.8,
	
	    .layout_x = WB_WIDGET_CENTER,
	    .layout_y = WB_WIDGET_CENTER,
	    .direction = WB_WIDGET_LEFT_TO_RIGHT,
	
	    .childs = container_children,
	    .nchild = 3,
	};

	wb_widget_rect(&root);


    wb_render_layout(fonts);

    cairo_surface_write_to_png(surface, "clay_cairo_test.png");

    cairo_destroy(c);
    cairo_surface_destroy(surface);

    puts("generated clay_cairo_test.png");
    return 0;
}

