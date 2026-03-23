#ifndef WBRO_MODULE
#define WBRO_MODULE


struct wb_public_api {
};

struct wb_widget_rect_data;
struct wb_widget_text_data;

struct wb_render;
struct wb_context;
struct wb_interest;
struct wb_appstate;

struct wb_style_sec;
struct wb_style_unit;
struct wb_style_base;
struct wb_style_main;

struct wb_config_setting;

struct wb_event {
	int fd;
	int event;
	void * data;
};

struct module_interface {
	char module_name[64];
	/*
	 * id is assigned by 
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
	 * parse custom style
	 */
	void (* parse_sty)(struct wb_config_setting * set, struct wb_style_main  * mstyle, 
						struct wb_style_base * base);

	int (* get_fd )(struct wb_context * ctx);

	/*
	 * prepare for the first emit
	 * return module state
	 * followed by emit_layout
	 */
	void * (* set_up)(struct wb_context * ctx);

	/*
	 * update module state
	 */
	void (* handle_event)(struct wb_event * event, struct wb_context * ctx,
					void * state);

	/* 
	 * emit module layout from state
	 */
	void (* emit_layout)(struct wb_context * ctx, void * state);

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
	int (* trigger_update)(struct wb_context * ctx);

	struct wb_poll_handle * (* reg_sub)(struct wb_context * ctx, int fd,
									int wevent, void * udata, int id);

	int (* rmv_sub)(struct wb_context * ctx, struct wb_poll_handle * handle);

};

struct wb_widget_api {
	void (* draw_rect)(struct wb_widget_rect_data * data);

	void (* draw_text)(struct wb_widget_text_data * data);
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
