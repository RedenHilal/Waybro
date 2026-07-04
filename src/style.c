#include "style.h"
#include "config.h"
#include "macro.h"
#include "module.h"

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

const struct wb_config_api config_api = {
	.parse_config = wb_style_parse_config
};

unsigned int wb_style_type_size[] = {
	[WB_STYLE_INT]		= sizeof(int),
	[WB_STYLE_LL]		= sizeof(long long),
	[WB_STYLE_BOOL]		= sizeof(int),
	[WB_STYLE_FLOAT]	= sizeof(double),
	[WB_STYLE_STRING]	= sizeof(char *),
	[WB_STYLE_CUSTOM]	= sizeof(void *)
};

static const struct config_dispatch main_table[] = {
	{
		.field_name = "width",
		.default_int = 1920,
		.field_type = WB_STYLE_INT,
		.offset = offsetof(struct wb_style_main, width)
	},
	{
		.field_name = "height",
		.default_int = 30,
		.field_type = WB_STYLE_INT,
		.offset = offsetof(struct wb_style_main, height)
	},
	{
		.field_name = "fonts",
		.default_a_str = (const char *[]){"Times New Roman", NULL},
		.array_size = 1,
		.field_type = WB_STYLE_STRING,
		.field_modifier = WB_STYLE_ARRAY,
		.offset = offsetof(struct wb_style_main, fonts),
		.size_offset = offsetof(struct wb_style_main, font_option_size)
	},
	{
		.field_name = "padding",
		.default_a_int = (const int []){0, 0, 0, 0},
		.field_type = WB_STYLE_CUSTOM,
		.custom_parse = wb_style_parse_side_styles,
		.offset = offsetof(struct wb_style_main, padding)
	},
	{
		.field_name = "fill_color",
		.default_int = 0xffffffff,
		.field_type = WB_STYLE_INT,
		.offset = offsetof(struct wb_style_main, fill_color)
	},
	{
		.field_name = "font_size",
		.default_int = 10,
		.field_type = WB_STYLE_INT,
		.offset = offsetof(struct wb_style_main, font_size)
	},
	{
		.field_name = "text_color",
		.default_int = 0x0,
		.field_type = WB_STYLE_INT,
		.offset = offsetof(struct wb_style_main, text_color)
	},
	{
		.field_name = "radius",
		.default_int = 0,
		.field_type = WB_STYLE_INT,
		.offset = offsetof(struct wb_style_main, radius)
	},
	{
		.field_name = "border_width",
		.default_a_int = (const int []){0, 0, 0, 0},
		.field_type = WB_STYLE_CUSTOM,
		.custom_parse = wb_style_parse_side_styles,
		.offset = offsetof(struct wb_style_main, border_width)
	},
	{
		.field_name = "border_color",
		.default_int = 0x0,
		.field_type = WB_STYLE_INT,
		.offset = offsetof(struct wb_style_main, border_color)
	},
	{
		.field_name = "group_gap",
		.default_int = 5,
		.field_type = WB_STYLE_INT,
		.offset = offsetof(struct wb_style_main, group_gap)
	},
	{
		.field_name = "group_color",
		.default_int = 0x0,
		.field_type = WB_STYLE_INT,
		.offset = offsetof(struct wb_style_main, group_color)
	},
	{
		.field_name = "group_radius",
		.default_int = 0,
		.field_type = WB_STYLE_INT,
		.offset = offsetof(struct wb_style_main, group_radius)
	},
	{
		.field_name = "group_border_width",
		.default_a_int = (const int []){0, 0, 0, 0},
		.field_type = WB_STYLE_CUSTOM,
		.custom_parse = wb_style_parse_side_styles,
		.offset = offsetof(struct wb_style_main, group_border_width)
	},
	{
		.field_name = "group_border_color",
		.default_int = 0x0,
		.field_type = WB_STYLE_INT,
		.offset = offsetof(struct wb_style_main, group_border_color)
	},
	{
		.field_name = "module_gap",
		.default_int = 10,
		.field_type = WB_STYLE_INT,
		.offset = offsetof(struct wb_style_main, module_gap)
	},
	{
		.field_name = "module_fill_color",
		.default_int = 0x0,
		.field_type = WB_STYLE_INT,
		.offset = offsetof(struct wb_style_main, module_fill_color)
	},
	{
		.field_name = "module_hover_color",
		.default_int = 0x0,
		.field_type = WB_STYLE_INT,
		.offset = offsetof(struct wb_style_main, module_hover_color)
	},
	{
		.field_name = "module_active_color",
		.default_int = 0x0,
		.field_type = WB_STYLE_INT,
		.offset = offsetof(struct wb_style_main, module_active_color)
	},
	{
		.field_name = "module_radius",
		.default_int = 0,
		.field_type = WB_STYLE_INT,
		.offset = offsetof(struct wb_style_main, module_radius)
	},
	{
		.field_name = "module_border_width",
		.default_a_int = (const int []){0, 0, 0, 0},
		.field_type = WB_STYLE_CUSTOM,
		.custom_parse = wb_style_parse_side_styles,
		.offset = offsetof(struct wb_style_main, module_border_width)
	},
	{
		.field_name = "module_border_color",
		.default_int = 0x0,
		.field_type = WB_STYLE_INT,
		.offset = offsetof(struct wb_style_main, module_border_color)
	},
	{
		.field_name = "module_padding",
		.default_a_int = (const int []){0, 0, 0, 0},
		.field_type = WB_STYLE_CUSTOM,
		.custom_parse = wb_style_parse_side_styles,
		.offset = offsetof(struct wb_style_main, module_padding)
	}
};

static const struct config_dispatch base_table[] = {
	{
		.field_name = "width",
		.default_int = 70,
		.field_type = WB_STYLE_INT,
		.offset = offsetof(struct wb_style_base, width)
	},
	{
		.field_name = "margin_left",
		.default_int = 0,
		.field_type = WB_STYLE_INT,
		.offset = offsetof(struct wb_style_base, margin_left)
	},
	{
		.field_name = "margin_right",
		.default_int = 0,
		.field_type = WB_STYLE_INT,
		.offset = offsetof(struct wb_style_base, margin_right)
	}
};

static void
log_config_value(void * target, const struct config_dispatch * dispatch)
{
	printf("Config %s:\n", dispatch->field_name);

	switch(dispatch->field_type) {
		case WB_STYLE_INT:
				printf("%d\n", *(int *)target);
				break;
		case WB_STYLE_LL:
				printf("%ld\n", *(long long int *)target);
				break;
		case WB_STYLE_BOOL:
				printf("%s\n", *(int *)target? "True": "False");
				break;
		case WB_STYLE_FLOAT:
				printf("%f\n", *(double *)target);
				break;
		case WB_STYLE_STRING:
				printf("%s\n", *(char **)target);
				break;
		case WB_STYLE_CUSTOM:
				break;
	}
}

static void
log_config_array(void ** target, const struct config_dispatch * dispatch, int length)
{
	
	char * ptr = *target;

	for (int i = 0; i < length; i++){
		log_config_value(ptr, dispatch);
		ptr += wb_style_type_size[dispatch->field_type];
	}
}

/*
 * TODO
 * add type safety for scalar and nonscalar data type
 */
static void
wb_style_assign_field(void * start, const struct config_dispatch * dispatch,
			struct wb_config_setting * mod_set)
{
	if (dispatch->field_type == WB_STYLE_CUSTOM) {
		return dispatch->custom_parse(mod_set, dispatch, start);
	}

	char * target = (char *)start;
	int res = wb_config_s_lookup(mod_set, dispatch->field_name,
					target, dispatch->field_type);

	if (res < 0) {
		memcpy(target, &dispatch->default_int, wb_style_type_size[dispatch->field_type]);
	}

	log_config_value(target, dispatch);
}

/*
 * TODO
 * handle array length passing
 * actually scalar data types were passed as well.
 *
 * moreover, setting returned from wb_config_s_get_setting
 * must be freed, and the implementation is yet to be made.
 */
static void
wb_style_handle_nonscalar(void * start, const struct config_dispatch * dispatch,
				struct wb_config_setting * mod_set)
{
	char * target = start;
	char * size_target = start;
	struct wb_config_setting * elm;
	target += dispatch->offset;

	int res;
	void ** arr = (void **)target;
	
	switch (dispatch->field_modifier){
		case WB_STYLE_ARRAY:
				size_target += dispatch->size_offset;
				elm = wb_config_s_get_setting(mod_set, dispatch->field_name);

				if (elm == NULL)
						goto default_arr;

				res = wb_config_parse_array(elm, arr, dispatch->field_type);

				if (res <= 0)
						goto default_arr;

				printf("length = %d\n", res);
				int * arr_size = (int *)size_target;
				*arr_size = res;
				log_config_array(arr, dispatch, res);
				break;
		case WB_STYLE_LIST:
				/*
				 * TODO
				 * idk how should i implement list parsing
				 * let's just put custom parse as for now
				 */
				dispatch->custom_parse(mod_set, dispatch, target);
				//if (!wb_config_is_list(mod_set))
				//		goto invalid_type;

				break;
		default:
				wb_style_assign_field(target, dispatch, mod_set);
				break;
	}
	return;

	default_arr:
	*arr = dispatch->default_ptr;
}

/*
 * TODO
 * Refactor style function
 * main_style allocated dynamically
 * but base_style allocated statically
 */
struct wb_style_main *
wb_style_get_main(struct wb_config_setting * mod_set)
{
	struct wb_style_main * msty = malloc(sizeof(struct wb_style_main));
	if (msty == NULL)
		ON_ERR("Failed allocation on main style")

	int nfield = sizeof(main_table) / sizeof(main_table[0]);
	
	for (int i = 0; i < nfield; i++){
			wb_style_handle_nonscalar(msty, &main_table[i], mod_set);
	}
	return msty;

}

void
wb_style_get_base(struct wb_style_base * base, struct wb_config_setting * mod_set,
				struct wb_style_main * msty)
{
    
    int iter = sizeof(base_table)/sizeof(base_table[0]);

    for(int i = 0; i < iter; i++){
		wb_style_handle_nonscalar(base, &base_table[i], mod_set);
    }

}

void
wb_style_parse_config(struct config_dispatch * dp, int length,
				void * start, struct wb_config_setting * mod_set)
{
	for (int i = 0; i < length; i++){
		wb_style_handle_nonscalar(start, &dp[i], mod_set);
	}
}

/*
 * used for styles which have 4 side characteristic
 * such as padding, border, etc
 */

static void
assign_default_side_styles(void * start, const struct config_dispatch * dispatch)
{
	memcpy(start, dispatch->default_ptr, sizeof(int) * 4);
}

static int
parse_size_arr(void * start, const struct config_dispatch * dispatch,
				struct wb_config_setting * setting)
{

	int length = wb_config_s_length(setting);
	int val[4];
	int res;

	switch (length) {
		case 1:
			res = wb_config_elem_by_index(setting, val, WB_STYLE_INT, 0);
			val[1] = val[2] = val[3] = val[0];
			break;
		case 2:
			for (int i = 0; i < length; i++) {
				res = wb_config_elem_by_index(setting, &val[i], WB_STYLE_INT, i);
				if (res < 0) {
					return -1;
				}
			}

			val[2] = val[0];
			val[3] = val[1];
			break;
		case 4:
			for (int i = 0; i < length; i++) {
				res = wb_config_elem_by_index(setting, &val[i], WB_STYLE_INT, i);
				if (res < 0) {
					return -1;
				}
			}

			break;
		default:
			return -1;
	}

	memcpy(start, val, sizeof(int) * 4);
	return 0;
}

static int
parse_side_scalar(void * start, const struct config_dispatch * dispatch,
				struct wb_config_setting * mod_set)
{
	int val;
	int res = wb_config_s_lookup(mod_set, dispatch->field_name, &val, WB_STYLE_INT);
	if (res < 0) {
		return -1;
	}

	int * target = start;
	target[0] = target[1] = target[2] = target[3] = val;
	return 0;
}

void
wb_style_parse_side_styles(struct wb_config_setting * mod_set,
				const struct config_dispatch * dispatch, void * start)
{
	struct wb_config_setting * setting;
	setting = wb_config_s_get_setting(mod_set, dispatch->field_name);
	int res;

	if (setting == NULL) {
		assign_default_side_styles(start, dispatch);
		return ;
	}

	if (wb_config_is_array(setting)) {
		res = parse_size_arr(start, dispatch, setting);
	} else if (wb_config_is_number(setting)) {
		res = parse_side_scalar(start, dispatch, mod_set);

	} else {
		res = -1;
	}

	if (res < 0) {
		LOG_CRIT("Invalid %s configuration\n", dispatch->field_name);
		exit(1);
	}
	
	return;
}
