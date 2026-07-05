#include "fetcher.h" 

#include "widget.h"
#include "module.h"
#include "macro.h"

#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include <pulse/context.h>
#include <pulse/mainloop.h>
#include <pulse/subscribe.h>
#include <pulse/def.h>
#include <pulse/volume.h>
#include <pulse/introspect.h>

int get_volume_fd(struct wb_context * ctx);
void pa_parse_sty(struct wb_config_setting * set, struct wb_style_main * msty,
				struct wb_style_base * base);
void volume_get(struct wb_event * event, struct wb_context * ctx, void * state);
void * get_volume_data(struct wb_context * ctx);
void pa_render(struct wb_context * ctx, void * state);

struct pa_state {
	pa_context * context;
	pa_mainloop * ml;
	pa_mainloop_api * mla;

	int vol_now;
	char sink_name[64];
	char text[64];
	int pipe;
};

struct pa_cb_render_data {
	struct pa_state * state;
	struct wb_context * ctx;
};

static struct module_interface mod = {
	.module_name	= "pulseaudio",
	.parse_sty		= pa_parse_sty,
	.get_fd			= get_volume_fd,
	.set_up			= get_volume_data,
	.handle_event	= volume_get,
	.emit_layout	= pa_render,
	.clean_up		= NULL
};

static pthread_t tid;
static int retval;

const struct module_interface *
mod_init()
{
	return &mod;
}

static void
pa_render_text(void * data)
{
	struct pa_cb_render_data * cd = data;
	const struct wb_public_api * api = mod.api;

	struct wb_widget_text_data text = api->widget->default_text(cd->ctx);
	text.string = cd->state->text;

	api->widget->text(cd->ctx, &text);
}

static const struct wb_widget_callback pa_cb = {

};

void
pa_render(struct wb_context * ctx, void * data)
{
	struct pa_state * state = data;
	const struct wb_public_api * api = mod.api;
	static int id = -1;

	struct pa_cb_render_data cb_render_data = {state, ctx};

	if (id < 0) {
		id = api->widget->allocate_id(ctx);
		api->widget->set_id(ctx, id, state, WB_POINTER_HOVER, &pa_cb);
	}

	int event = api->widget->get_event(ctx, id);
	struct wb_widget_rect_special rect = {
		.rect = api->widget->default_rect(ctx, event)
	};

	rect.rect.data = &cb_render_data;
	rect.rect.child_cb = pa_render_text;

	api->widget->bind_id(ctx, id, &rect);
	api->widget->rect_special(ctx, &rect);
}

void
pa_parse_sty(struct wb_config_setting * set, struct wb_style_main * msty,
				struct wb_style_base * base)
{
	
}

static void
pa_sink_info_cb(pa_context * c, const pa_sink_info * i, int eol, void * data)
{
	if(eol){
		return;
	}
	
	struct pa_state * state = data;

	pa_volume_t vol_raw = pa_cvolume_avg(&i->volume);
	int vol_now = (vol_raw * 100) / PA_VOLUME_NORM;
	if (state->vol_now == vol_now) {
		return;
	}

	state->vol_now = vol_now;
	snprintf(state->text, 64, "%d%%", vol_now);

	write(state->pipe, &vol_now, sizeof(int));
}

static void
pa_server_info_cb(pa_context * c, const pa_server_info * i, void * data)
{
	struct pa_state * state = data;
	const char * default_sink = i->default_sink_name;
	strncpy(state->sink_name, default_sink, 64);

	pa_operation * op = pa_context_get_sink_info_by_name(c, default_sink,
					pa_sink_info_cb, data);	

}

static void
pa_subscribe_cb(pa_context * c, pa_subscription_event_type_t t,
				uint32_t idx, void * data)
{

	t = t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK;
	printf("triggered subscribe\n");
	switch (t){
		case PA_SUBSCRIPTION_EVENT_SINK:
			pa_context_get_server_info(c, pa_server_info_cb, data);
	}
}

static void
pa_success_cb(pa_context * c, int success, void * data)
{
	pa_context_set_subscribe_callback(c, pa_subscribe_cb, data);
}

static void
pa_state_cb(pa_context * c, void * data)
{
	pa_context_state_t state = pa_context_get_state(c);

	LOG_INFO("Pulseaudio ready cb triggered\n");

	switch (state){
		case PA_CONTEXT_READY:		
			pa_context_get_server_info(c, pa_server_info_cb, data);
			pa_context_subscribe(c, PA_SUBSCRIPTION_MASK_SINK, pa_success_cb, data);
			break;
		case PA_CONTEXT_FAILED:
			ON_ERR("pa_failed");
		default:
			// idk
	}
}

static void *
pa_listener(void * data)
{
	struct pa_state * state = data;

	pa_mainloop * ml = pa_mainloop_new();    
	pa_mainloop_api * mla = pa_mainloop_get_api(ml);
	pa_context * pactx = pa_context_new(mla, "waybro");

	state->context = pactx;
	state->ml = ml;
	state->mla = mla;

	pa_context_set_state_callback(pactx, pa_state_cb, state);
	if(pa_context_connect(pactx, NULL, 0, NULL) < 0) 
			ON_ERR("pa_connect")
	if(pa_mainloop_run(ml, &retval) < 0)
			ON_ERR("pa ml run thread")

	return NULL;
}

int
get_volume_fd(struct wb_context * ctx)
{
    
	int pipes[2];
	if(pipe(pipes) != 0)
			ON_ERR("pipe pulse")

	struct pa_state * state = malloc(sizeof(struct pa_state));
	state->pipe = pipes[1];
	mod.data = state;

	return pipes[0];
}

void *
get_volume_data(struct wb_context * ctx)
{
	pthread_create(&tid, NULL, pa_listener, mod.data);
	return mod.data;
}


void
volume_get(struct wb_event * event, struct wb_context * ctx, void * state)
{
	const struct wb_public_api * api = mod.api;
	int vol;

	read(event->fd, &vol, sizeof(int));
	api->mod->trigger_update(ctx);
}
