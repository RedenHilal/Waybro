#include <stdlib.h>
#include <stdio.h>

#include "uthash.h"
#include "style.h"
#include "module.h"
#include "api-table.h"

const struct wb_style_api wb_style_api_table = {
	.get_str = wb_style_get_str,
	.get_int = wb_style_get_int,
	.get_float = wb_style_get_float,
	.get_base = wb_style_get_base,
	.str_by_format = wb_style_str_by_format
};

char * wb_style_get_str(struct wb_style_sec * sec, const char * key)
{
	struct wb_style_unit * sty = NULL;
	HASH_FIND_STR(sec->style, key, sty);

	if (sty == NULL)
		return NULL;

	return sty->str_val;
}


int wb_style_get_int(struct wb_style_sec * sec, const char * key)
{
	struct wb_style_unit * sty = NULL;
	HASH_FIND_STR(sec->style, key, sty);

	if (sty == NULL)
		return -1;

	return sty->int_val;
}


double wb_style_get_float(struct wb_style_sec * sec, const char * key)
{
	struct wb_style_unit * sty = NULL;
	HASH_FIND_STR(sec->style, key, sty);

	if (sty == NULL)
		return -1;

	return sty->db_val;
}

void wb_style_get_base(struct wb_style_base * base, struct wb_style_sec * sec,
						struct wb_style_main * msty)
{
	get_base_sty(base, sec, msty);
}

char * wb_style_str_by_format(char * format, char * str_val){
    char * begin = strchr(format, '{');
    char * end = strchr(begin, '}');

    int pre_len = begin - format;
    int post_len = strlen(end + 1);
    int val_len = strlen(str_val);

    int total_len = pre_len + post_len + val_len + 1;
    char * str_fmt = malloc(total_len);

    snprintf(str_fmt, total_len, "%.*s%s%s", pre_len, format, str_val, end);

    return str_fmt;
}
