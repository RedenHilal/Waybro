#include <libconfig.h>
#include <stdlib.h>
#include <stdio.h>

#include "style.h"
#include "config.h"
#include "macro.h"


struct wb_config {
	config_t cfg;
};

struct wb_config_setting {
	config_setting_t * setting;
};

/*
 * TODO
 * generalize return value for normal return and err
 */
void
wb_config_get_paths(char ** paths)
{
	const char * user_path = getenv("HOME");
	const char * config_path = ".config/waybro/pconfig";

	char * user_config = malloc(WB_CONFIG_PATH_LENGTH);
	paths[0] = user_config;
	
	snprintf(paths[0], WB_CONFIG_PATH_LENGTH, "%s/%s", user_path, config_path);
	printf("%s\n", paths[0]);
	paths[1] = NULL;
	
}

void
wb_config_err_detail(struct wb_config * wcfg)
{
	config_t * cfg = &wcfg->cfg;
	fprintf(stderr, "%s:%d - %s\n", config_error_file(cfg),
					config_error_line(cfg), config_error_text(cfg));
}

struct wb_config *
wb_config_init(char ** paths)
{
	struct wb_config * wcfg = malloc(sizeof(struct wb_config));
	if (wcfg == NULL)
		ON_ERR("Config Allocation Failed")
	
	config_init(&wcfg->cfg);

	while (*paths != NULL && config_read_file(&wcfg->cfg, *paths) != CONFIG_TRUE){
		paths++;
	}

	if (*paths == NULL)
		ON_ERR("Failed to get config")

	return wcfg;
}

struct wb_config_setting *
wb_config_get_setting(struct wb_config * wcfg, const char * path)
{
	config_setting_t * ret = config_lookup(&wcfg->cfg, path);

	struct wb_config_setting * setting;
	setting = malloc(sizeof(struct wb_config_setting));
	if (setting == NULL)
			return NULL;

	setting->setting = ret;
	return setting;
}

struct wb_config_setting *
wb_config_s_get_setting(struct wb_config_setting * set, const char * path)
{
	config_setting_t * s = set->setting;
	config_setting_t * ret = config_setting_lookup(s, path);

	struct wb_config_setting * setting;
	setting = malloc(sizeof(struct wb_config_setting));
	if (setting == NULL)
			return NULL;

	setting->setting = ret;
	return setting;
}

struct wb_config_setting *
wb_config_s_by_index(struct wb_config_setting * set, int index)
{
	config_setting_t * ret = config_setting_get_elem(set->setting, index);

	struct wb_config_setting * setting;
	setting = malloc(sizeof(struct wb_config_setting));
	if (setting == NULL)
			return NULL;

	setting->setting = ret;
	return setting;
}

int
wb_config_s_length(struct wb_config_setting * cfg_set)
{
	return config_setting_length(cfg_set->setting);
}

int
wb_config_s_is_valid(struct wb_config_setting * set)
{
	return set->setting != NULL;
}

const char *
wb_config_s_name(struct wb_config_setting * set)
{
	return config_setting_name(set->setting);
}

int
wb_config_is_array(struct wb_config_setting * set)
{
	return config_setting_is_array(set->setting);
}

int
wb_config_lookup(struct wb_config * wcfg, const char * path, void * start, int type)
{
	int res;
	config_t * cfg = &wcfg->cfg;

	switch (type) {
		case WB_STYLE_INT:
				res = config_lookup_int(cfg, path, start);
				break;
		case WB_STYLE_LL:
				res = config_lookup_int64(cfg, path, start);
				break;
		case WB_STYLE_BOOL:
				res = config_lookup_bool(cfg, path, start);
				break;
		case WB_STYLE_FLOAT:
				res = config_lookup_float(cfg, path, start);
				break;
		case WB_STYLE_STRING:
				res = config_lookup_string(cfg, path, start);
				break;
		default:
				res = -1;
				break;
	}

	if (res != CONFIG_TRUE)
			return -1;
	return 0;
}

/*
 * TODO
 * change the usage of wb_config_s_lookup* into this function
 */
int
wb_config_s_lookup(struct wb_config_setting * set, const char * path,
				void * start, int type)
{
	int res;
	config_setting_t * s = set->setting;

	switch(type) {
		case WB_STYLE_INT:
				res = config_setting_lookup_int(s, path, start);
				break;
		case WB_STYLE_LL:
				res = config_setting_lookup_int64(s, path, start);
				break;
		case WB_STYLE_BOOL:
				res = config_setting_lookup_bool(s, path, start);
				break;
		case WB_STYLE_FLOAT:
				res = config_setting_lookup_float(s, path, start);
				break;
		case WB_STYLE_STRING:
				res = config_setting_lookup_string(s, path, start);
				break;
		default:
				res = -1;
				break;
	}

	if (res != CONFIG_TRUE)
			return -1;
	return 0;
}

int
wb_config_elem_by_index(struct wb_config_setting * set, void * start, int type, int index)
{
	config_setting_t * elm = config_setting_get_elem(set->setting, index);
	int res;

	if (elm == NULL)
			return -1;

	switch (type) {
		case WB_STYLE_INT:
				res = config_setting_get_int_safe(elm, start);
				break;
		case WB_STYLE_LL:
				res = config_setting_get_int64_safe(elm, start);
				break;
		case WB_STYLE_BOOL:
				res = config_setting_get_bool_safe(elm, start);
				break;
		case WB_STYLE_FLOAT:
				res = config_setting_get_float_safe(elm, start);
				break;
		case WB_STYLE_STRING:
				res = config_setting_get_string_safe(elm, start);
				break;
	}

	if (res == CONFIG_FALSE)
			return -1;

	return 0;
}

int
wb_config_parse_array(struct wb_config_setting * set, void ** data, int type)
{
	if (config_setting_is_array(set->setting) == CONFIG_FALSE)
			return -1;

	int res;
	int length = config_setting_length(set->setting);
	if (length <= 0)
			return length;

	void * arr = malloc(wb_style_type_size[type] * length);

	if (arr == NULL)
			return -1;

	for (int i = 0; i < length; i++){
		res = wb_config_elem_by_index(set, &arr[i], type, i);
		if (res < 0)
				goto clean_fail;
	}

	*data = arr;
	return length;

	clean_fail:
	free(arr);
	return -1;
}
