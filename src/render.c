#include <stdlib.h>

#include "renderer/clay_renderer_cairo.c"
#include "macro.h"
#include "widget.h"
#include "render.h"
#include "comm.h"
#include "core.h"
#include "style.h"

//const struct wb_widget_api wb_widget_api_table[] = {
//	.draw_rect = wb_widget_rect,
//	.draw_text = wb_widget_text
//};

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
	Clay_Initialize(mem_arena, (Clay_Dimensions) {width, height}, (Clay_ErrorHandler) {HandleClayErrors});
}

void
wb_layout_context_init(struct wb_render * wrender)
{
	Clay_Cairo_Initialize(wrender->cai_context);
}

void
wb_layout_font_init(char * fonts[])
{
	Clay_SetMeasureTextFunction(Clay_Cairo_MeasureText, fonts);
}

void
wb_layout_begin()
{
	Clay_BeginLayout();
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

	for (int i = 0; i < ilist->ncount; i++){
		Clay_String str_id = {
				.isStaticallyAllocated = true,
				.length = strlen(ilist->node[i].id),
				.chars = ilist->node[i].id
		};

		Clay_ElementId id = Clay_GetElementId(str_id);
		Clay_ElementData element = Clay_GetElementData(id);
		Clay_BoundingBox bb = element.boundingBox;

		wl_region_add(ctx->appstate->region, bb.x, bb.y, bb.width, bb.height);
	}
}

void 
wb_widget_region_clean(struct wb_context * ctx)
{
	struct wb_style_main * msty = ctx->msty;
	wl_region_subtract(ctx->appstate->region, 0, 0, msty->width, msty->height);
}

static void
wb_widget_listen_clean(struct wb_context * ctx)
{
	struct wb_widget_interest_list * ilist = ctx->ilist;
	ilist->ncount = 0;
}

/*
 * helper
 */

static int
wb_widget_listen_insert(struct wb_context * ctx, struct wb_widget_listen_node * node)
{
	struct wb_widget_interest_list * ilist = ctx->ilist;

	/*
	 * Special widget count exceed WB_WIDGET_INTEREST_SIZE
	 */
	if (ilist->ncount == WB_WIDGET_INTEREST_SIZE)
		return -1;

	ilist->node[ilist->ncount++] = *node;
	return 0;
}

static Clay_SizingAxis
get_clay_size(enum wb_widget_sizing_type type, union wb_widget_sizing_unit val)
{
	switch (type){
			case WB_WIDGET_GROW:
					return CLAY_SIZING_GROW(0);
			case WB_WIDGET_FIXED:
					return CLAY_SIZING_FIXED(val.size);
			case WB_WIDGET_PERCENT:
					return CLAY_SIZING_PERCENT(val.percent);
			default:
					return CLAY_SIZING_FIT();
	}
}


/*
 * shapes
 */


/*
 * TODO
 * Implement sizing correctly
 */
void wb_widget_rect(struct wb_widget_rect_basic * data)
{
	struct wb_widget_color fc = data->fill_color;
	struct wb_widget_color bc = data->border_color;
	struct wb_widget_padding p = data->padding;
	int border_width = data->border_width;
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
			.width = {border_width, border_width, border_width, border_width}
		},
		.cornerRadius = {radius, radius, radius, radius},
	}){
		if (data->child_cb)
			data->child_cb(data->data);
	}
}

int
wb_widget_rect_special_(struct wb_widget_rect_special * data,
				struct wb_context * ctx)
{
	wb_widget_rect(&data->rect);
	struct wb_widget_listen_node * node  = malloc(sizeof(
							struct wb_widget_listen_node));
	if (node == NULL)
		return -1;
	
	*node = data->event;
	if (wb_widget_listen_insert(ctx, node) < 0)
		return -1;

	return 0;
}

void
wb_widget_text(struct wb_widget_text_data * data)
{
	struct wb_widget_color tc = data->text_color;
	Clay_String text = {
		.isStaticallyAllocated = false,
		.length = strlen(data->string),
		.chars = data->string
	};
		CLAY_TEXT(text,
			CLAY_TEXT_CONFIG({
					.fontSize = data->font_size,
					.textAlignment = (Clay_TextAlignment)data->alignment,
					.wrapMode = (Clay_TextElementConfigWrapMode)data->wrap,
					.textColor = (Clay_Color){tc.r, tc.g, tc.b, tc.a}
			})
		);
	};


void
wb_widget_scrollable()
{

}

void
wb_widget_popup()
{

}

void
wb_layout_compute(struct wb_context * ctx)
{
	Clay_RenderCommandArray commands = wb_layout_end();

	wb_widget_listen_clean(ctx);
	wb_widget_region_init(ctx);

	wb_render_layout(commands, ctx->msty->fonts);
}





