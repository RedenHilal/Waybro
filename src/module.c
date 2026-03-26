#include <unistd.h>
#include <stdlib.h>

#include "comm.h"
#include "module.h"
#include "poll.h"

struct
wb_poll_handle * wb_mod_reg_sub(struct wb_context * ctx, int fd,
										int wevent, void * udata, int id);

int
wb_mod_rmv_sub(struct wb_context * ctx, struct wb_poll_handle * handle);

void
wb_mod_trigger_update(struct wb_context * ctx);

const struct wb_mod_api mod_api = {
	.trigger_update = wb_mod_trigger_update,
	.reg_sub = wb_mod_reg_sub,
	.rmv_sub = wb_mod_rmv_sub
};

struct
wb_poll_handle * wb_mod_reg_sub(struct wb_context * ctx, int fd,
										int wevent, void * udata, int id)
{
	struct wb_poll_handle * handle;
	struct wb_event_packet * ev_packet = malloc(sizeof(struct wb_event_packet));

	if (ev_packet = NULL)
		return NULL;

	handle = wb_poll_reg_events(ctx->fort, fd, wevent, ev_packet);
	ev_packet->handle = handle;
	ev_packet->udata = udata;
	ev_packet->mod_int = ctx->mod_int[id];

	return handle;
}

int
wb_mod_rmv_sub(struct wb_context * ctx, struct wb_poll_handle * handle)
{
	void * packet = wb_poll_data_from_handle(handle);
	free(packet);
	
	return wb_poll_rmv_events(handle);
}

void
wb_mod_trigger_update(struct wb_context * ctx)
{
	int pload = 1;
	write(ctx->pipe, &pload, sizeof(int));
}
