#ifndef WBRO_COMM
#define WBRO_COMM

#include <semaphore.h>
#include <pthread.h>

struct wb_appstate;
struct wb_poll_fort;
struct module_interface;
struct wb_poll_handle;
struct wb_layout;
struct wb_render;

struct wb_widget_interest_list;
struct wb_appstate;

struct wb_pointer_state {
	struct {
		double x, y;
	} pos;

	/*
	 * flag wheter hit test should be done on next frame
	 */
	int check;

	/*
	 * widget_id which the last press hit
	 * it shall have < 0 value if no widget was hit
	 */
	int po_id;
};

struct wb_frame_state {
	int update;
	int state_change;
};

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

	struct wb_pointer_state * ptr;

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
	struct wb_frame_state * frame;
	struct wb_render * render;
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
