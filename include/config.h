#ifndef WBRO_CONFIG
#define WBRO_CONFIG

#define WB_CONFIG_PATHS_NUM 2
#define WB_CONFIG_PATH_LENGTH 128

struct wb_config;
struct wb_config_setting;

void
wb_config_get_paths(char ** paths);

/*
 * takes null terminated string array
 * each string represent possible config path
 */
struct wb_config *
wb_config_init(char ** path);

void
wb_config_err_detail(struct wb_config * wcfg);

struct wb_config_setting *
wb_config_get_setting(struct wb_config * wcfg, const char * path);

struct wb_config_setting *
wb_config_s_get_setting(struct wb_config_setting * wcfg, const char * path);

struct wb_config_setting *
wb_config_s_by_index(struct wb_config_setting * setting, int index);

int
wb_config_s_length(struct wb_config_setting * setting);

int
wb_config_s_length(struct wb_config_setting * set);

const char *
wb_config_s_name(struct wb_config_setting * set);

int
wb_config_is_array(struct wb_config_setting * set);

/*
 * retrieval
 * type refers to enum wb_style_type in style.h
 */
int
wb_config_lookup(struct wb_config * wcfg, const char * path, void * start, int type);

int
wb_config_s_lookup(struct wb_config_setting * set, const char * path, void * start,
				int type);

int
wb_config_elem_by_index(struct wb_config_setting * set, void * start, int type, int index);

int
wb_config_parse_array(struct wb_config_setting * set, void ** data, int type);

#endif
