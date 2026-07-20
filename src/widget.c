#include <stdlib.h>

#include "comm.h"
#include "widget.h"
#include "module.h"
#include "macro.h"
#include "style.h"

int 
wb_widget_allocate_id(struct wb_context * ctx)
{
	static int curr_id = 0;

	if (ctx->ilist->ncount >= WB_WIDGET_INTEREST_SIZE) {
		return -1;
	}

	struct wb_widget_listen_node * node;

	node = calloc(1, sizeof(struct wb_widget_listen_node));
	if (node == NULL) {
		LOG_CRIT("Ran out of memory");
		exit(1);
		return -1;
	}

	node->on_click = NULL;
	node->on_scroll = NULL;
	ctx->ilist->ncount++;

	if (ctx->ilist->fs_count > ctx->ilist->fs_index) {

		int idx = ctx->ilist->fs[ctx->ilist->fs_index++];
		ctx->ilist->node[idx] = node;
		node->id = idx;
		LOG_INFO("Reused Id: %d\n", idx);
		return idx;
	}
	else {
		ctx->ilist->node[curr_id] = node;
		node->id = curr_id;
		LOG_INFO("Allocated Id: %d\n", curr_id);
		return curr_id++;
	}
}

struct wb_widget_listen_node *
wb_widget_get_event(struct wb_context * ctx, int id)
{
	if (id >= WB_WIDGET_INTEREST_SIZE || id < 0) {
		return NULL;
	}

	return ctx->ilist->node[id];
}


/*
 * TODO
 * alter integer param into pointer handle
 * for better safety
 */
int
wb_widget_free_id(struct wb_context * ctx, int id)
{
	if (ctx->ilist->fs_count >= WB_WIDGET_FREE_SLOT_SIZE) {
		return -1;
	}

	LOG_INFO("Freed Id: %d\n", id);
	ctx->ilist->ncount--;
	ctx->ilist->fs[ctx->ilist->fs_count++] = id;

	free(ctx->ilist->node[id]);
	ctx->ilist->node[id] = NULL;

	return 0;
}

int
wb_widget_get_widget_event(struct wb_context * ctx, int id)
{
	if (id > WB_WIDGET_INTEREST_SIZE || id < 0) {
		return WB_WIDGET_INVALID;
	}

	struct wb_widget_listen_node * node;
	node = ctx->ilist->node[id];

	return node->events;
}

int
wb_widget_hit_single_widget(struct wb_context * ctx, double x, double y, int event)
{
	struct wb_widget_interest_list * ilist = ctx->ilist;
	struct wb_widget_listen_node * node = NULL, * target = NULL;
	int widget_count = ilist->wf_count - 1;

	for (int i = widget_count; i >= 0; i--) {
		int widget_id = ilist->wf_index[i];
		node = ilist->node[widget_id];

		/*
		 * handle hyprland release event on workspace change
		 */
		if (node == NULL) {
			continue;
		}

		if (node->event_mask & event == 0) {
			continue;
		}

		if (x >= node->left && x <= node->right &&
			y >= node->top && y <= node->bottom) {
			node->events |= event;
			return node->id;
		}
	}

	return -1;
}

int
wb_widget_hit_multiple_widget(struct wb_context * ctx, double x, double y, int event)
{
	struct wb_widget_interest_list * ilist = ctx->ilist;
	struct wb_widget_listen_node * node = NULL;
	int widget_count = ilist->wf_count - 1;
	int hit_count = 0;

	for (int i = widget_count; i >= 0; i--) {
		int widget_id = ilist->wf_index[i];
		node = ilist->node[widget_id];

		if (node->event_mask & event == 0) {
			continue;
		}

		if (x >= node->left && x <= node->right &&
			y >= node->top && y <= node->bottom) {
			if (node->events & event) {
				continue;
			}

			node->events |= event;
			hit_count++;
		}
		else if (node->events & event) {
			node->events &= ~event;
			hit_count++;
		}
	}

	return hit_count;
}

int
wb_widget_set_id(struct wb_context * ctx, int id, void * data, int events,
				const struct wb_widget_callback * cb)
{
	if (id > WB_WIDGET_INTEREST_SIZE || id < 0) {
		return -1;
	}

	struct wb_widget_listen_node * node = ctx->ilist->node[id];
	if (node == NULL) {
		return -1;
	}

	if (events & WB_POINTER_BUTTON) {
		if (cb->on_click == NULL) {
			return -1;
		}

		node->on_click = cb->on_click;
	}

	node->event_mask = events;
	node->data = data;
}

int
wb_widget_bind_id(struct wb_context * ctx, int id, struct wb_widget_rect_special * rect)
{
	if (id > WB_WIDGET_INTEREST_SIZE || id < 0) {
		return -1;
	}

	struct wb_widget_listen_node * node = ctx->ilist->node[id];
	if (node == NULL) {
		return -1;
	}

	rect->event = node;
	return 0;
}

struct wb_widget_rect_basic
wb_widget_get_default_rect(struct wb_context * ctx, int event)
{
	struct wb_style_main * msty = ctx->msty;
	int is_hovered = event & WB_POINTER_HOVER;
	int fill_color = msty->module_fill_color;

	int * pad = msty->module_padding;
	int * bw = msty->module_border_width;


	if (event & WB_POINTER_BUTTON) {
		fill_color = msty->module_active_color;
	}
	else if (event & WB_POINTER_HOVER) {
		fill_color = msty->module_hover_color;
	}

	struct wb_widget_rect_basic rect = {
		.sizing_height = WB_WIDGET_GROW,
		.sizing_width = WB_WIDGET_FIT,
		.fill_color = WB_COLOR_FROM_RGBA(fill_color),
		.layout_y = WB_WIDGET_CENTER,
		.radius = msty->radius,
		.padding = {pad[0], pad[1], pad[2], pad[3]},
		.border_width = {bw[0], bw[1], bw[2], bw[3]},
		.border_color = WB_COLOR_FROM_RGBA(msty->module_border_color)
	};

	return rect;
}

struct wb_widget_text_data
wb_widget_get_default_text(struct wb_context * ctx)
{
	struct wb_style_main * msty = ctx->msty;

	struct wb_widget_text_data text = {
		.font_size = msty->font_size,
		.text_color = WB_COLOR_FROM_RGBA(msty->text_color)
	};

	return text;
}

int
wb_widget_toggle_active(struct wb_context * ctx, int id, int active)
{
	if (id > WB_WIDGET_INTEREST_SIZE || id < 0) {
		return -1;
	}

	struct wb_widget_listen_node * node = ctx->ilist->node[id];
	if (node == NULL) {
		return -1;
	}

	if (active) {
		node->events |= WB_POINTER_BUTTON;
	} else {
		node->events &= ~WB_POINTER_BUTTON;
	}
	return 0;
}
