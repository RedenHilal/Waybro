#ifndef WBRO_MODULE
#define WBRO_MODULE

#include "wb-style-base.h"


struct wb_public_api {
	const struct wb_style_api * style;
	const struct wb_mod_api * mod;
	const struct wb_render_api * render;
};

struct wb_render;
struct wb_context;
struct wb_interest;

struct wb_style_sec;
struct wb_style_unit;
struct wb_style_base;
struct wb_style_main;


struct wb_event {
	int fd;
	int event;
	void * data;
};

struct wb_data {
	int id;
	union {
		char str_val[64];
		int int_val;
		double db_val;
	};
	// it is the responsible of the module to handle this had this be used
	void * data;
};


struct module_interface {
	char module_name[64];
	/*
	 * id is assigned through mod_init
	 */
	int id;

	/*
	 * module cache pointer
	 * module-owned (its the module responsibility to free it)
	 */
	void * data;

	struct wb_public_api * api;

	/*
	 * module are free to define their own struct
	 * wb_style_base must be embedded as the first member
	 * module-owned
	 */
	void * style;

	/*
	 * module must clean up rstyle
	 */
	void (* parse_sty)(struct wb_style_sec * rstyle, struct wb_style_main  * mstyle);

	int (* get_fd )(struct wb_context * ctx);

	//send first message to populate the bar information
	void (* set_up)(struct wb_context * ctx);

	void (* handle_event)(struct wb_event * event, struct wb_context * ctx);

	void (* handle_update)(struct wb_render * render, struct wb_data * data);

	void (* clean_up)();
};


/*
 * module entry point
 * struct module_interface * mod_init(int id, struct wb_public_api * api);
 */


/*
 * public api
 */

struct wb_mod_api {
	int (* send_data)(struct wb_context * ctx, struct wb_data * data);

	struct wb_poll_handle * (* reg_sub)(struct wb_context * ctx, int fd,
									int wevent, void * udata, int id);

	int (* rmv_sub)(struct wb_context * ctx, struct wb_poll_handle * handle);

};

struct wb_render_api {
	void (* draw_rect)(struct wb_render * wrender, int x, int y,
					int width, int height);

	void (* expose_area)(struct wb_render * render, int x, int y,
					int width, int height);

	void (* erase_area)(struct wb_render * wrender, int x, int y,
					int width, int height);

	void (* draw_text)(struct wb_render * wrender, int x, int y,
					const char * string);

	void (* draw_text_special)(struct wb_render * wrender, int x, int y,
					 const char * string);

};

struct wb_style_api {
	char * (* get_str)(struct wb_style_sec * sec, const char * key);

	int (* get_int)(struct wb_style_sec * sec, const char * key);

	double (* get_float)(struct wb_style_sec * sec, const char * key);

	void (* get_base)(struct wb_style_base * base, struct wb_style_sec * sec,
					struct wb_style_main * msty);

	char * (* str_by_format)(char * format, char * str_val);

};


#endif
