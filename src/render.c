#include <stdlib.h>

#include "renderer/clay_renderer_cairo.c"
#include "macro.h"
#include "widget.h"
#include "render.h"
#include "comm.h"
#include "core.h"
#include "style.h"

#include "module.h"
#include "assert.h"

const struct wb_widget_api widget_api = {
	.rect = wb_widget_rect,
	.text = wb_widget_text,
	.rect_special = wb_widget_rect_special,
	.allocate_id = wb_widget_allocate_id,
	.get_event = wb_widget_get_widget_event,
	.free_id = wb_widget_free_id,
	.set_id = wb_widget_set_id,
	.bind_id = wb_widget_bind_id,
	.default_rect = wb_widget_get_default_rect,
	.default_text = wb_widget_get_default_text,
	.toggle_active = wb_widget_toggle_active
};

static void
HandleClayErrors(Clay_ErrorData errorData)
{
    printf("%s", errorData.errorText.chars);
}

void
wb_layout_arena_allocate(int width, int height)
{
	u64 mem_size = Clay_MinMemorySize();
	Clay_Arena mem_arena = Clay_CreateArenaWithCapacityAndMemory
										(mem_size, malloc(mem_size));
	Clay_Initialize(mem_arena, (Clay_Dimensions) {width, height},
					(Clay_ErrorHandler) {HandleClayErrors});

}

void
wb_layout_context_init(struct wb_render * wrender)
{
	Clay_Cairo_Initialize(wrender->cai_context);
}

void
wb_layout_font_init(char * fonts[], int length)
{
	fonts[length] = NULL;
	Clay_SetMeasureTextFunction(Clay_Cairo_MeasureText, fonts);
}

void
wb_layout_begin(struct wb_context * ctx)
{
	struct wb_style_main * msty = ctx->msty;
	struct wb_widget_interest_list * ilist = ctx->ilist;

	Clay_SetLayoutDimensions((Clay_Dimensions){msty->width, msty->height});
	Clay_Context * clay_ctx = Clay_GetCurrentContext();
	Clay_BeginLayout();
	Clay_Dimensions d = clay_ctx->layoutDimensions;

	memset(ilist->frame_slot, 0, sizeof(int) * WB_WIDGET_FRAME_SLOT_SIZE);
	ilist->wf_count = 0;
}

static Clay_RenderCommandArray
wb_layout_end()
{
	Clay_RenderCommandArray commands = Clay_EndLayout();
	return commands;
}

static void
wb_render_layout(Clay_RenderCommandArray commands, char * fonts[])
{
	Clay_Cairo_Render(commands, fonts);
}

static void
wb_widget_region_init(struct wb_context * ctx)
{
	struct wb_widget_interest_list * ilist = ctx->ilist;
	int count = ilist->wf_count - 1;

	for (int i = count; i >= 0; i--){

		struct wb_widget_listen_node * node = ilist->node[i];
		if (node == NULL) {
			continue;
		}

		Clay_String str_id = {
				.isStaticallyAllocated = true,
				.length = 3,
				.chars = "spc"
		};

		Clay_ElementId id = Clay_GetElementIdWithIndex(str_id, i);
		Clay_ElementData element = Clay_GetElementData(id);
		Clay_BoundingBox bb = element.boundingBox;

		node->left = bb.x;
		node->right = bb.x + bb.width;
		node->top = bb.y;
		node->bottom = bb.y + bb.height;
	}
}

void
wb_widget_print_widget(char * id, int index)
{
	Clay_String str_id = {
		.isStaticallyAllocated = true,
		.length = strlen(id),
		.chars = id
	};

	Clay_ElementId eid = Clay_GetElementIdWithIndex(str_id, index);
	Clay_ElementData element = Clay_GetElementData(eid);
	if (element.found){
		Clay_BoundingBox bb = element.boundingBox;
		printf("Found widget %s-%d | x:%f y:%f w:%f h:%f\n", id, index, bb.x, bb.y,
						bb.width, bb.height);
	}
	else {
		printf("Failed to find widget %s-%d\n", id, index);
	}

}


void 
wb_widget_region_clean(struct wb_context * ctx)
{
	struct wb_style_main * msty = ctx->msty;
	struct wl_region * region = wl_compositor_create_region(ctx->appstate->compositor);
}

void
wb_widget_listen_clean(struct wb_context * ctx)
{
	struct wb_widget_interest_list * ilist = ctx->ilist;
	ilist->ncount = 0;
}

/*
 * helper
 */

static int
check_free_slot(int * frame_slot, int id)
{
	
	LOG_INFO("id %d\n", id);
	if (id >= WB_WIDGET_INTEREST_SIZE || id < 0) {
		return -1;
	}

	/*
	 * frame slot is used
	 */
	if (frame_slot[id / WB_WIDGET_INT_BITS] & (1 << (id % WB_WIDGET_INT_BITS))) {
		return -1;
	}

	return 0;
}

static int
wb_widget_listen_insert(struct wb_context * ctx, struct wb_widget_listen_node * node)
{
	struct wb_widget_interest_list * ilist = ctx->ilist;
	int id = node->id;

	if (check_free_slot(ilist->frame_slot, id) < 0) {
		return -1;
	}
	LOG_INFO("WF_COUNT %d\n", ilist->wf_count);

	ilist->frame_slot[id / WB_WIDGET_INT_BITS] |= (1 << (id & WB_WIDGET_INT_BITS));
	ilist->wf_index[ilist->wf_count++] = id;


	ilist->node[id] = node;

	return 0;
}

static Clay_SizingAxis
get_clay_size(enum wb_widget_sizing_type type, union wb_widget_sizing_unit val)
{
	switch (type){
			case WB_WIDGET_GROW:
					return CLAY_SIZING_GROW(val.min, val.max);
			case WB_WIDGET_FIXED:
					return CLAY_SIZING_FIXED(val.size);
			case WB_WIDGET_PERCENT:
					return CLAY_SIZING_PERCENT(val.percent);
			case WB_WIDGET_FIT:
					return CLAY_SIZING_FIT(val.min, val.max);
			default:
					return CLAY_SIZING_FIT(val.min, val.max);
	}
}

/*
 * TODO
 * Implement sizing correctly
 */
void wb_widget_rect(struct wb_context * ctx, struct wb_widget_rect_basic * data)
{
	struct wb_widget_color fc = data->fill_color;
	struct wb_widget_color bc = data->border_color;
	struct wb_widget_padding p = data->padding;
	struct wb_widget_border_width bw = data->border_width;
	int radius = data->radius;
	Clay_Sizing sizing = {
		.width = get_clay_size(data->sizing_width, data->width),
		.height = get_clay_size(data->sizing_height, data->height)
	};

	CLAY_AUTO_ID({
		.layout = {
			.sizing = sizing,
			.childGap = data->child_gap,
			.childAlignment = {
				.x = data->layout_x,
				.y = data->layout_y
			},
			.padding = (Clay_Padding){
				.left = p.left,
				.right = p.right,
				.top = p.top,
				.bottom = p.bottom
			},
			.layoutDirection = data->direction
		},
		.backgroundColor = (Clay_Color){fc.r, fc.g, fc.b, fc.a},
		.border = {
			.color = (Clay_Color){bc.r, bc.g, bc.b, bc.a},
			.width = {bw.left, bw.right, bw.top, bw.bottom}
		},
		.cornerRadius = {radius, radius, radius, radius},
	}){
		if (data->child_cb)
			data->child_cb(ctx, data->data);
	};
}

int
wb_widget_rect_special(struct wb_context * ctx, struct wb_widget_rect_special * rect)
{
	struct wb_widget_rect_basic * data = &rect->rect;
	struct wb_widget_color fc = data->fill_color;
	struct wb_widget_color bc = data->border_color;
	struct wb_widget_padding p = data->padding;

	struct wb_widget_border_width bw = data->border_width;
	int radius = data->radius;

	Clay_Sizing sizing = {
		.width = get_clay_size(data->sizing_width, data->width),
		.height = get_clay_size(data->sizing_height, data->height)
	};

	CLAY(CLAY_IDI("spc", rect->event->id), {
		.layout = {
			.sizing = sizing,
			.childGap = data->child_gap,
			.childAlignment = {
				.x = data->layout_x,
				.y = data->layout_y
			},
			.padding = (Clay_Padding){
				.left = p.left,
				.right = p.right,
				.top = p.top,
				.bottom = p.bottom
			},
			.layoutDirection = data->direction
		},
		.backgroundColor = (Clay_Color){fc.r, fc.g, fc.b, fc.a},
		.border = {
			.color = (Clay_Color){bc.r, bc.g, bc.b, bc.a},
			.width = {bw.left, bw.right, bw.top, bw.bottom}
		},
		.cornerRadius = {radius, radius, radius, radius},
	}){
		if (data->child_cb)
			data->child_cb(ctx, data->data);
	};

	if (wb_widget_listen_insert(ctx, rect->event) < 0)
		return -1;

	return 0;
}

int
wb_widget_rect_with_id(struct wb_context * ctx, struct wb_widget_rect_basic * data,
				char * id, int index)
{
	struct wb_widget_color fc = data->fill_color;
	struct wb_widget_color bc = data->border_color;
	struct wb_widget_padding p = data->padding;
	struct wb_widget_border_width bw = data->border_width;
	int radius = data->radius;
	Clay_Sizing sizing = {
		.width = get_clay_size(data->sizing_width, data->width),
		.height = get_clay_size(data->sizing_height, data->height)
	};

	Clay_String str_id = {
		.isStaticallyAllocated = true,
		.length = strlen(id),
		.chars = id
	};

	CLAY(CLAY_SIDI(str_id, index), {
		.layout = {
			.sizing = sizing,
			.childGap = data->child_gap,
			.childAlignment = {
				.x = data->layout_x,
				.y = data->layout_y
			},
			.padding = (Clay_Padding){
				.left = p.left,
				.right = p.right,
				.top = p.top,
				.bottom = p.bottom
			},
			.layoutDirection = data->direction
		},
		.backgroundColor = (Clay_Color){fc.r, fc.g, fc.b, fc.a},
		.border = {
			.color = (Clay_Color){bc.r, bc.g, bc.b, bc.a},
			.width = {bw.left, bw.right, bw.top, bw.bottom}
		},
		.cornerRadius = {radius, radius, radius, radius},
	}){
		if (data->child_cb)
			data->child_cb(ctx, data->data);
	};
}

void
wb_widget_text(struct wb_context * ctx, struct wb_widget_text_data * data)
{
	struct wb_style_main * msty = ctx->msty;
	int font_size = data->font_size <= 0? msty->font_size : data->font_size;
	
	struct wb_widget_color tc = data->text_color;
	Clay_String text = {
		.isStaticallyAllocated = false,
		.length = strlen(data->string),
		.chars = data->string
	};

	CLAY_TEXT(text,
		CLAY_TEXT_CONFIG({
				.fontSize = font_size,
				.textAlignment = (Clay_TextAlignment)data->alignment,
				.wrapMode = (Clay_TextElementConfigWrapMode)data->wrap,
				.textColor = (Clay_Color){tc.r, tc.g, tc.b, tc.a}
		})
	);
}


void
wb_widget_scrollable()
{

}

void
wb_widget_popup()
{

}

static void
wb_widget_log_bar()
{
	wb_widget_print_widget("main", 0);

	for (int i = 0; i < 3; i++){
		wb_widget_print_widget("group", i);
	}
}

void
wb_layout_compute(struct wb_context * ctx)
{
	struct wb_style_main * msty = ctx->msty;
	struct wb_appstate * appstate = ctx->appstate;

	Clay_RenderCommandArray commands = wb_layout_end();

//	wb_widget_log_bar();
	wb_widget_region_init(ctx);
	wb_render_layout(commands, ctx->msty->fonts);

	wl_surface_attach(appstate->surface, appstate->buffer, 0, 0);
	wl_surface_damage(appstate->surface, 0, 0, msty->width, msty->height);
	wl_surface_commit(appstate->surface);
}


