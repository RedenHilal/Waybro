#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "comm.h"
#include "module.h"
#include "poll.h"
#include "widget.h"

struct
wb_poll_handle * wb_mod_reg_sub(struct wb_context * ctx, int fd,
										int wevent, void * udata, int id);

int
wb_mod_rmv_sub(struct wb_context * ctx, struct wb_poll_handle * handle);

void
wb_mod_trigger_update(struct wb_context * ctx);

void
wb_mod_state_change(struct wb_context * ctx);

int
wb_mod_sub_text(const char * format, const char * label, char * result,
				const void * target, int type, int length);

void *
wb_mod_data_from_handle(struct wb_context * ctx, struct wb_poll_handle * handle);

const struct wb_mod_api mod_api = {
	.trigger_update = wb_mod_state_change,
	.reg_sub = wb_mod_reg_sub,
	.rmv_sub = wb_mod_rmv_sub,
	.sub_text = wb_mod_sub_text,
	.data_from_handle = wb_mod_data_from_handle
};

struct
wb_poll_handle * wb_mod_reg_sub(struct wb_context * ctx, int fd,
										int wevent, void * udata, int id)
{
	struct wb_poll_handle * handle;
	struct wb_event_packet * ev_packet = malloc(sizeof(struct wb_event_packet));

	if (ev_packet == NULL)
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
	struct wb_event_packet * packet = wb_poll_data_from_handle(handle);
	free(packet);

	return wb_poll_rmv_events(handle);
}

void *
wb_mod_data_from_handle(struct wb_context * ctx, struct wb_poll_handle * handle)
{
	void * data = wb_poll_data_from_handle(handle);
	return data;
}

void
wb_mod_state_change(struct wb_context * ctx)
{
	ctx->frame->state_change = 1;
}

int
wb_mod_sub_text(const char * format, const char * label, char * result,
				const void * target, int type, int length)
{
	int label_length = strlen(label);
	if (label_length > 64 - 2) {
		return -1;
	}

	char buffer[64];
	snprintf(buffer, sizeof(buffer), "%c%s%c", '{', label, '}');

	const char * found = strstr(format, buffer);
	if (found == NULL) {
		return -1;
	}

	long offset = (long)found - (long)format;
	int resume = offset + label_length + 2;

	switch(type) {
		case WB_MOD_INT:
			snprintf(result, length, "%.*s%d%s", offset, format,
							*(int *)target, format + resume);
			break;
		case WB_MOD_LL:
			snprintf(result, length, "%.*s%lld%s", offset, format,
							*(long long int *)target, format + resume);
			break;
		case WB_MOD_FLOAT:
			snprintf(result, length, "%.*s%f%s", offset, format,
							*(double *)target, format + resume);
			break;
		case WB_MOD_STRING:
			snprintf(result, length, "%.*s%s%s", offset, format,
							(char *) target, format + resume);
			break;
		default:
	}
	
	return 0;
}
