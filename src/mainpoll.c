#include "fetcher.h"
#include "style.h"
#include "core.h"
#include "macro.h"
#include "module.h"
#include "render.h"
#include "poll.h"
#include "comm.h"


static void set_wpoll(struct module_interface ** interfaces, int mod_count,
					struct wb_context * ctx, struct wb_poll_fort * fort){

    for(int i = 0; i < mod_count; i++){
		struct wb_event_packet * packet = malloc(sizeof(struct wb_event_packet));
		if (packet == NULL)
			ON_ERR("packet allocation failed")

		int fd = interfaces[i]->get_fd(ctx);
		if (fd < 0)
			ON_ERR("Invalid fd")

		packet->mod_int = interfaces[i];
		packet->handle = wb_poll_reg_events(fort, fd, WB_EVENT_READ, packet);

    }
}

static void handle_events(struct wb_poll_fort * fort, struct wb_poll_event * event,
							struct wb_context * ctx){
	struct wb_event_packet * packet;
	while ((packet = wb_poll_reap_event(fort, event)) != NULL){
		struct module_interface * interface = packet->mod_int;
		int fd = event->fd;

		struct wb_event wb_event = {
			.fd = fd,
			.event = event->ev_mask
		};
		interface->handle_event(&wb_event, ctx);
	}
}

static void module_setup(struct module_interface ** interfaces, int mod_count,
						struct wb_context * ctx){
	for (int i = 0; i < mod_count; i++){
		interfaces[i]->set_up(ctx);
	}
}

void * mainpoll(void * data){

    struct wb_poll_event events[MAKS_EVENT];
    struct module_context * mod_ctx = data; 

	int mod_count = mod_ctx->module_count;
	int pipe = mod_ctx->pipe;
	struct module_interface ** interfaces = mod_ctx->interfaces;

	struct wb_poll_fort * fort = wb_poll_create(O_CLOEXEC, WB_EVENT_EDGE);

	struct wb_context wb_ctx = {
			.mod_int = interfaces, 
			.pipe = pipe, 
			.fort = fort
	};

    set_wpoll(interfaces, mod_count, &wb_ctx, fort);

	module_setup(interfaces, mod_count, &wb_ctx);

    while (1) {
        int ready = wb_poll_wait_events(fort, events, MAKS_EVENT, -1);

        for(int i = 0; i < ready; i++){
			handle_events(fort, &events[i], &wb_ctx);
        }
    }
}
