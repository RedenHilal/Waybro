#ifndef WBRO_COMM
#define WBRO_COMM

#include <semaphore.h>
#include <pthread.h>

struct wb_appstate;
struct wb_poll_fort;
struct module_interface;
struct wb_poll_handle;
struct wb_layout;

struct wb_widget_interest_list;
struct wb_appstate;

struct module_context {
	int pipe;
	int module_count;

	struct module_interface ** interfaces;
	struct wb_config_setting ** sets;
	struct wb_poll_handle ** handles;

	void ** states;
	pthread_mutex_t * mutexes;

	struct wb_context * ctx;

	/*
	 * semaphore for modules set_up completion wait
	 */
	sem_t * sem;
};

/*
 * TODO
 * split struct to each thread if possible
 */
struct wb_context {
	/*
	 * both thread
	 */
	struct module_interface ** mod_int;

	/*
	 * data piping - mainloop thread
	 */
	int pipe;
	struct wb_poll_fort * fort;

	/*
	 * rendering - render thread
	 */
	struct wb_style_main * msty;
	struct wb_appstate * appstate;
	struct wb_widget_interest_list * ilist;
	struct wb_layout * layout;
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
