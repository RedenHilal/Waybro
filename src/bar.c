#include "widget.h"
#include "style.h"
#include "comm.h"
#include "core.h"
#include "layout.h"
#include "module.h"

struct module_group {
	int index;
	struct wb_layout_node * head;
	struct module_context * mod_ctx;
};

static void
wb_bar_modules_cb(void * data)
{
	struct module_group * group = data;
	struct module_context * mod_ctx = group->mod_ctx;
	struct wb_layout_node * ptr = group->head;
	struct module_interface ** interfaces = mod_ctx->ctx->mod_int;

	void ** states = mod_ctx->states;
	pthread_mutex_t * mutexes = mod_ctx->mutexes;
	
	while (ptr){
		int id = ptr->id;
		struct module_interface * mod_int = interfaces[id];

		pthread_mutex_lock(&mutexes[id]);
		mod_int->emit_layout(mod_ctx->ctx, states[id]);
		pthread_mutex_unlock(&mutexes[id]);
	}
	
}

static void
wb_bar_group_cb(void * data)
{
	struct module_context * mod_ctx = data;
	struct wb_context * ctx = mod_ctx->ctx;
	struct wb_layout * layout = ctx->layout;
	struct wb_layout_node * head = layout->lhead;

	int group_sizing[3] = {
		WB_WIDGET_FIT,
		WB_WIDGET_GROW,
		WB_WIDGET_FIT
	};

	struct wb_widget_rect_basic group = {0};

	group.child_cb = wb_bar_modules_cb;
	group.data = mod_ctx;

	/*
	 * start on head as lhead
	 * increment to chead, and rhead
	 * by advancing the pointer with head++
	 */
	for (int i = 0; i < 3; i++){
		struct module_group data = {
				.index = i,
				.head = head++,
				.mod_ctx = mod_ctx
		};
		group.sizing_width = group_sizing[i];
		wb_widget_rect(&group);
	}

}

static void
wb_bar_main(struct module_context * mod_ctx)
{
	struct wb_context * ctx = mod_ctx->ctx;
	struct wb_style_main * msty = ctx->msty;

	struct wb_widget_rect_basic main = {
		.fill_color = WB_COLOR_FROM_RGBA(msty->fill_color),
		.border_color = WB_COLOR_FROM_RGBA(msty->border_color),
		.padding = WB_PADDING_ALL(msty->padding),
		.border_width = msty->border_width,
		.radius = msty->padding,
		.child_gap = 0,
		.sizing_width = WB_WIDGET_FIXED,
		.sizing_height = WB_WIDGET_FIXED,
		.width = {.size = msty->width},
		.height = {.size = msty->height},
		.layout_x = WB_WIDGET_CENTER,
		.layout_y = WB_WIDGET_CENTER,
		.direction = WB_WIDGET_LEFT_TO_RIGHT,

		.child_cb = wb_bar_group_cb,
		.data = mod_ctx
	};

	wb_widget_rect(&main);
}


void
wb_bar_render(struct module_context * mod_ctx)
{
	struct wb_context * ctx = mod_ctx->ctx;
	struct wb_style_main * msty = ctx->msty;

	wb_widget_region_clean(ctx);

	wb_layout_begin();

	wb_bar_main(mod_ctx);
	
	wb_layout_compute(ctx);
}

void
wb_bar_init(struct module_context * mod_ctx)
{
	struct wb_context * ctx = mod_ctx->ctx;
	struct wb_style_main * msty = ctx->msty;

	wb_layout_font_init(msty->fonts);
}
