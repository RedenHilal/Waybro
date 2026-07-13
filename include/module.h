#ifndef WBRO_MODULE
#define WBRO_MODULE

struct wb_public_api {
	const struct wb_mod_api * mod;
	const struct wb_widget_api * widget;
	const struct wb_config_api * config;
};

struct wb_poll_handle;

struct wb_widget_rect_basic;
struct wb_widget_text_data;
struct wb_widget_rect_special;
struct wb_widget_callback;

struct config_dispatch;

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

enum wb_mod_type {
	WB_MOD_INT,
	WB_MOD_LL,
	WB_MOD_BOOL,
	WB_MOD_FLOAT,
	WB_MOD_STRING,
	WB_MOD_CUSTOM
};

struct module_interface {
	char module_name[64];
	/*
	 * id is assigned by backend
	 */
	int id;

	/*
	 * module cache pointer
	 * module-owned (its the module responsibility to free it)
	 */
	void * data;


	/*
	 * assigned by backend
	 */
	const struct wb_public_api * api;
	struct wb_style_base * base_style;
	void * custom_style;

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
 * public api
 */

struct wb_mod_api {

	void (* trigger_update)(struct wb_context * ctx);

	struct wb_poll_handle * (* reg_sub)(struct wb_context * ctx, int fd, int wevent,
					void * udata, int id);

	int (* rmv_sub)(struct wb_context * ctx, struct wb_poll_handle * handle);

	int (* sub_text)(const char * format, const char * label, char * result,
					const void * target, int type, int length);

	void * (* data_from_handle)(struct wb_context * ctx,
					struct wb_poll_handle * handle);
};

struct wb_widget_api {

	void (* rect)(struct wb_context * ctx, struct wb_widget_rect_basic * data);

	void (* text)(struct wb_context * ctx, struct wb_widget_text_data * data);

	int (* rect_special)(struct wb_context * ctx, struct wb_widget_rect_special * data);

	int (* allocate_id)(struct wb_context * ctx);

	int (* get_event)(struct wb_context * ctx, int id);

	int (* free_id)(struct wb_context * ctx, int id);

	int (* set_id)(struct wb_context * ctx, int id, void * data, int events,
					const struct wb_widget_callback * cb);

	int (* bind_id)(struct wb_context * ctx, int id,
					struct wb_widget_rect_special * rect);

	struct wb_widget_rect_basic (* default_rect)(struct wb_context * ctx, int events);

	struct wb_widget_text_data (* default_text)(struct wb_context * ctx);

	int (* toggle_active)(struct wb_context * ctx, int id, int active);

};

struct wb_config_api {

	void (* parse_config)(const struct config_dispatch * dp, int length, void * start,
					struct wb_config_setting * set);

};


#endif
