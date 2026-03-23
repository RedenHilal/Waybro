#include "fetcher.h"
#include "style.h"
#include "core.h"
#include "macro.h"
#include "module.h"
#include "render.h"
#include "poll.h"
#include "comm.h"


static void set_wpoll(struct module_context * mod_ctx,
					struct wb_context * ctx, struct wb_poll_fort * fort){

	int mod_count = mod_ctx->module_count;
    for(int i = 0; i < mod_count; i++){
		struct module_interface * interface = mod_ctx->interfaces[i];

		int fd = interface->get_fd(ctx);
		if (fd < 0)
			ON_ERR("Invalid fd")

		mod_ctx->handles[i] = wb_poll_reg_events(fort, fd, WB_EVENT_READ, interface);

    }
}

static void handle_events(struct module_context * mod_ctx, struct wb_poll_fort * fort,
				struct wb_poll_event * event, struct wb_context * ctx){

	void * data;
	while ((data = wb_poll_reap_event(fort, event)) != NULL){
		struct module_interface * interface = data;
		int id = interface->id;
		int fd = event->fd;

		struct wb_event wb_event = {
			.fd = fd,
			.event = event->ev_mask
		};

		pthread_mutex_lock(&mod_ctx->mutexes[id]);
		interface->handle_event(&wb_event, ctx, mod_ctx->states[id]);
		pthread_mutex_unlock(&mod_ctx->mutexes[id]);
	}
}

static void module_setup(struct module_context * mod_ctx, struct wb_context * ctx){
	int mod_count = mod_ctx->module_count;
	for (int i = 0; i < mod_count; i++){
		mod_ctx->states[i] = mod_ctx->interfaces[i]->set_up(ctx);
	}
}

void * mainpoll(void * data){

    struct wb_poll_event events[MAKS_EVENT];
    struct module_context * mod_ctx = data; 
	sem_t * sem = mod_ctx->sem;

	int mod_count = mod_ctx->module_count;
	int pipe = mod_ctx->pipe;
	struct module_interface ** interfaces = mod_ctx->interfaces;

	struct wb_poll_fort * fort = wb_poll_create(O_CLOEXEC, WB_EVENT_EDGE);

	struct wb_context wb_ctx = {
			.mod_int = interfaces, 
			.pipe = pipe, 
			.fort = fort
	};

    set_wpoll(mod_ctx, &wb_ctx, fort);

	module_setup(mod_ctx, &wb_ctx);
	sem_post(sem);

    while (1) {
        int ready = wb_poll_wait_events(fort, events, MAKS_EVENT, -1);

        for(int i = 0; i < ready; i++){
			handle_events(mod_ctx, fort, &events[i], &wb_ctx);
        }
    }
}
