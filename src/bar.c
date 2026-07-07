#include <unistd.h>

#include "widget.h"
#include "style.h"
#include "comm.h"
#include "core.h"
#include "layout.h"
#include "module.h"
#include "render.h"

struct module_group {
	int index;
	struct wb_layout_node * head;
	struct module_context * mod_ctx;
};

static void
wb_bar_modules_cb(struct wb_context * ctx, void * data)
{
	struct module_group * group = data;
	struct module_context * mod_ctx = group->mod_ctx;
	struct wb_layout_node * ptr = group->head;
	struct module_interface ** interfaces = mod_ctx->interfaces;

	void ** states = mod_ctx->states;
	pthread_mutex_t * mutexes = mod_ctx->mutexes;
	
	int i = 0;
	while (ptr){
		int id = ptr->id;
		struct module_interface * mod_int = interfaces[id];

		pthread_mutex_lock(&mutexes[id]);
		mod_int->emit_layout(mod_ctx->ctx, states[id]);
		pthread_mutex_unlock(&mutexes[id]);
		ptr = ptr->next;
	}
	
}

static void
wb_bar_group_cb(struct wb_context * ctx, void * data)
{
	struct wb_widget_rect_basic * group = data;

	wb_widget_rect(ctx, group);
}

static void
wb_bar_group_parent_cb(struct wb_context * ctx, void * data)
{
	struct module_context * mod_ctx = data;
	struct wb_style_main * msty = ctx->msty;
	struct wb_layout * layout = ctx->layout;
	struct wb_layout_node * head = layout->lhead;
	int * pad = msty->padding;

	struct wb_layout_node * heads[3] = {
		layout->lhead,
		layout->chead,
		layout->rhead
	};

	int group_sizing[3] = {
		WB_WIDGET_GROW,
		WB_WIDGET_FIXED,
		WB_WIDGET_GROW
	};

	int group_layout[3] = {
		WB_WIDGET_LEFT,
		WB_WIDGET_CENTER,
		WB_WIDGET_RIGHT
	};

	struct wb_widget_rect_basic parent = {
		.child_cb = wb_bar_group_cb,
		.sizing_height = WB_WIDGET_GROW
	};

	struct wb_widget_rect_basic group = {
		.padding = {pad[0], pad[1], pad[2], pad[3]},
		.child_cb = wb_bar_modules_cb,
		.radius = msty->radius,
		.sizing_height = WB_WIDGET_GROW,
		.sizing_width = WB_WIDGET_FIT,
		.width = {0},
		.height = {0},
		.direction = WB_WIDGET_LEFT_TO_RIGHT,
		.fill_color = WB_COLOR_FROM_RGBA(msty->group_color),
		.child_gap = msty->module_gap
	};

	/*
	 * start on head as lhead
	 * increment to chead, and rhead
	 * by advancing the pointer with head++
	 */
	for (int i = 0; i < 3; i++){
		struct module_group data = {
				.index = i,
				.head = heads[i],
				.mod_ctx = mod_ctx
		};

		parent.sizing_width = group_sizing[i];
		parent.data = &group;
		parent.layout_x = group_layout[i];

		group.layout_x = group_layout[i];
		group.data = &data;
		wb_widget_rect_with_id(ctx, &parent, "parent", i);
	}

}

static void
wb_bar_main(struct module_context * mod_ctx)
{
	struct wb_context * ctx = mod_ctx->ctx;
	struct wb_style_main * msty = ctx->msty;
	int * bw = msty->border_width;
	
	struct wb_widget_rect_basic main = {
		.fill_color = WB_COLOR_FROM_RGBA(msty->fill_color),
		.border_color = WB_COLOR_FROM_RGBA(msty->border_color),
		.border_width = {bw[0], bw[1], bw[2], bw[3]},
		.child_gap = msty->group_gap,
		.sizing_width = WB_WIDGET_FIXED,
		.sizing_height = WB_WIDGET_FIXED,
		.width = {.size = msty->width},
		.height = {.size = msty->height},
		.layout_x = WB_WIDGET_LEFT,
		.layout_y = WB_WIDGET_CENTER,
		.direction = WB_WIDGET_LEFT_TO_RIGHT,

		.child_cb = wb_bar_group_parent_cb,
		.data = mod_ctx
	};

	wb_widget_rect_with_id(ctx, &main, "main", 0);
}

void
wb_bar_render(struct module_context * mod_ctx)
{
	LOG_INFO("render here\n");
	struct wb_context * ctx = mod_ctx->ctx;
	wb_widget_print_widget("main",  0);

	wb_widget_region_clean(ctx);
	wb_widget_listen_clean(ctx);
	wb_layout_begin(ctx);

	wb_bar_main(mod_ctx);
	
	wb_layout_compute(ctx);

}

void
wb_bar_init(struct wb_render * wrender)
{
	struct wb_style_main * msty = wrender->m_style;

	wb_layout_context_init(wrender);
	wb_layout_font_init(msty->fonts, msty->font_option_size);
}

void
wb_bar_erase(struct wb_render * wrender)
{
	cairo_t * cr = wrender->cai_context;
	struct wb_style_main * msty = wrender->m_style;

	cairo_save(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
	cairo_rectangle(cr, 0, 0, msty->width, msty->height);
	cairo_fill(cr);
	cairo_restore(cr);

}

void
wb_bar_trigger_update(struct wb_context * ctx)
{
	int pload = 1;
	write(ctx->pipe, &pload, sizeof(int));
}
