#ifndef WBRO_COMM
#define WBRO_COMM

struct appstate;
struct wb_poll_fort;
struct module_interface;
struct wb_poll_handle;

struct module_context {
	int pipe;
	int module_count;
	struct appstate * appstate;
	struct module_interface ** interfaces;
};

struct wb_context {
	struct module_interface ** mod_int;
	int pipe;
	struct wb_poll_fort * fort;
};

struct wb_interest {
	int fd;
	int event;
};

struct wb_event_packet {
	struct wb_poll_handle * handle;
	struct module_interface * mod_int;
	void * udata;
};

#endif
