#ifndef WBRO_API_TABLE
#define WBRO_API_TABLE

#include "module.h"

int wb_mod_send_data(struct wb_context * ctx,  struct wb_data * data);

struct wb_poll_handle * wb_mod_reg_sub(struct wb_context * ctx,int fd, int wevent,
										void * udata, int id);

int wb_mod_rmv_sub(struct wb_context * ctx, struct wb_poll_handle * handle);

/* 
 * style parsing helper
 */
char * wb_style_get_str(struct wb_style_sec * sec, const char * key);
int wb_style_get_int(struct wb_style_sec * sec, const char * key);
double wb_style_get_float(struct wb_style_sec * sec, const char * key);

char * wb_style_str_by_format(char * format, char * str_val);
void wb_style_get_base(struct wb_style_base * base, struct wb_style_sec * sec,
						struct wb_style_main * msty);
/*
 * rendering utility
 */

void wb_render_draw_rect(struct wb_render * wrender, int x, int y,
						int width, int height);

void wb_render_expose_area(struct wb_render * wrender, int x, int y,
						int width, int height);

void wb_render_erase_area(struct wb_render * wrender, int x, int y,
						int width, int height);

void wb_render_draw_text(struct wb_render * wrender, int x, int y,
						const char * string);

void wb_render_draw_text_special(struct wb_render * wrender, int x, int y,
						const char * string);

/*
 * public api
 */
extern const struct wb_style_api wb_style_api_table;
extern const struct wb_mod_api wb_mod_api_table;
extern const struct wb_render_api wb_render_api_table;

#endif
