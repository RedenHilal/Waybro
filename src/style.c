#include "style.h"
#include "config.h"
#include "macro.h"

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

struct config_dispatch {
	char field_name[32];
	union {
		const char * default_str;
		const char ** default_a_str;
		const int default_int;
		const int * default_a_int;
		const double default_float;
		const double * default_a_float;
		void * default_ptr;
	};
	int array_size;
	void ( *custom_parse)(void * start, const struct config_dispatch * dispatch,
					struct wb_config_setting * mod_set);
	int offset;
	int field_type;
	int field_modifier;
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
		.offset = offsetof(struct wb_style_main, fonts)
	},
	{
		.field_name = "fill_color",
		.default_int = 0xffffffff,
		.field_type = WB_STYLE_INT,
		.offset = offsetof(struct wb_style_main, fill_color)
	},
	{
		.field_name = "font_size",
		.default_int = 12,
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
		.default_int = 0,
		.field_type = WB_STYLE_INT,
		.offset = offsetof(struct wb_style_main, border_width)
	},
	{
		.field_name = "border_color",
		.default_int = 0x0,
		.field_type = WB_STYLE_INT,
		.offset = offsetof(struct wb_style_main, border_color)
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
		.default_int = 0,
		.field_type = WB_STYLE_INT,
		.offset = offsetof(struct wb_style_main, group_border_width)
	},
	{
		.field_name = "group_border_color",
		.default_int = 0x0,
		.field_type = WB_STYLE_INT,
		.offset = offsetof(struct wb_style_main, group_border_color)
	},
	{
		.field_name = "module_radius",
		.default_int = 0,
		.field_type = WB_STYLE_INT,
		.offset = offsetof(struct wb_style_main, module_radius)
	},
	{
		.field_name = "module_border_width",
		.default_int = 0,
		.field_type = WB_STYLE_INT,
		.offset = offsetof(struct wb_style_main, module_border_width)
	},
	{
		.field_name = "module_border_color",
		.default_int = 0x0,
		.field_type = WB_STYLE_INT,
		.offset = offsetof(struct wb_style_main, module_border_color)
	},
	{
		.field_name = "module_color",
		.default_int = 0x0,
		.field_type = WB_STYLE_INT,
		.offset = offsetof(struct wb_style_main, module_color)
	},
	{
		.field_name = "module_color_hover",
		.default_int = 0x0,
		.field_type = WB_STYLE_INT,
		.offset = offsetof(struct wb_style_main, module_color_hover)
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
				printf("%s\n", (char *)target);
				break;
		case WB_STYLE_CUSTOM:
				break;
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
	char * target = (char *)start;

	int res = wb_config_s_lookup(mod_set, dispatch->field_name,
					target, dispatch->field_type);

	if (res < 0) {
		memcpy(target, &dispatch->default_int, wb_style_type_size[dispatch->field_type]);
	}

	log_config_value(target, dispatch);
}

/*
 * actually scalar data types were passed as well
 */
static void
wb_style_handle_nonscalar(void * start, const struct config_dispatch * dispatch,
				struct wb_config_setting * mod_set)
{
	char * target = start;
	struct wb_config_setting * elm;
	target += dispatch->offset;

	int res;
	void ** arr = (void **)&target;
	
	switch (dispatch->field_modifier){
		case WB_STYLE_ARRAY:
				res = wb_config_parse_array(mod_set, arr, dispatch->field_type);
				target = dispatch->default_ptr;
				break;
		case WB_STYLE_LIST:
				/*
				 * TODO
				 * idk implementation of list parsing
				 * let's just put custom parse as for now
				 */
				//if (!wb_config_is_list(mod_set))
				//		goto invalid_type;

				dispatch->custom_parse(target, dispatch, mod_set);
				break;
		default:
				wb_style_assign_field(target, dispatch, mod_set);
				break;
	}

	invalid_type:
	target = dispatch->default_ptr;
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
	printf("main fields: %d\n", nfield);
	
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
