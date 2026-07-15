#include <sys/timerfd.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <stdint.h>
#include <time.h>

#include "module.h"
#include "macro.h"
#include "widget.h"

#define TEXT_MAX 64

void clock_parse_sty(struct wb_config_setting * set, struct wb_style_main * msty, 
				struct wb_style_base * base); 
int get_time_fd(struct wb_context * ctx);
void * time_fd_init(struct wb_context * ctx);
void time_get(struct wb_event * event, struct wb_context * ctx, void * state);
void clock_render(struct wb_context * ctx, void * state);

struct clock_data {
	struct wb_context * ctx;
	struct clock_state * state;
};

struct clock_state {
	char text[64];
	int mode;
	struct tm time;
};

static struct module_interface mod = {
	.module_name	= "clock",
	.parse_sty		= clock_parse_sty,
	.get_fd			= get_time_fd,
	.set_up			= time_fd_init,
	.handle_event	= time_get,
	.emit_layout	= clock_render,
	.clean_up		= NULL
};

struct module_interface * mod_init(int id, struct wb_public_api * api){
	return &mod;
}

static void
draw_text(struct wb_context * ctx, void * udata)
{
	struct clock_data * data = udata;
	struct clock_state * state = data->state;
	const struct wb_public_api * api = mod.api;

	char * fmt = state->mode? "%a | %m %B" : "%H:%M";
	strftime(state->text, TEXT_MAX, fmt, &state->time);

	struct wb_widget_text_data text = api->widget->default_text(data->ctx);
	text.string = state->text;

	api->widget->text(data->ctx, &text);
}

void handle_click(struct wb_context * ctx, void * data)
{
	const struct wb_public_api * api = mod.api;
	struct clock_state * state = data;
	state->mode = !state->mode;

	api->mod->trigger_update(ctx);
}

static const struct wb_widget_callback clock_cb = {
	.on_click = handle_click
};

void clock_render(struct wb_context * ctx, void * data){
	const struct wb_public_api * api = mod.api;
	struct clock_state * state = data;
	static int id = -1;

	struct clock_data cb_data = {ctx, state};

	if (id < 0) {
		id = api->widget->allocate_id(ctx);
		api->widget->set_id(ctx, id, state, WB_POINTER_BUTTON | WB_POINTER_HOVER,
						&clock_cb);
	}
	
	int event = api->widget->get_event(ctx, id);
	struct wb_widget_rect_special rect = {
		.rect = api->widget->default_rect(ctx, event)
	};

	rect.rect.child_cb = draw_text;
	rect.rect.data = &cb_data;

	api->widget->bind_id(ctx, id, &rect);
	api->widget->rect_special(ctx, &rect);

}

void clock_parse_sty(struct wb_config_setting * set, struct wb_style_main * msty,
				struct wb_style_base * base){
		printf("clock parse\n");
}

void time_get(struct wb_event * event, struct wb_context * ctx, void * data){
	const struct wb_public_api * api = mod.api;
	struct clock_state * state = data;
    uint64_t trigger;

    read(event->fd, &trigger, sizeof(uint64_t));

	state->time.tm_min += trigger;
	mktime(&state->time);

	api->mod->trigger_update(ctx);
}

int get_time_fd(struct wb_context * ctx){
	struct clock_state * state = malloc(sizeof(struct clock_state));
	mod.data = state;
    int timefd = timerfd_create(CLOCK_REALTIME, 0);

    struct itimerspec timer;
    time_t date_now = time(NULL);
    struct tm * tmstat = localtime_r(&date_now, &state->time);
    
    timer.it_value.tv_sec = date_now + (60 - tmstat->tm_sec);
    timer.it_value.tv_nsec = 0;
    timer.it_interval.tv_sec = 60;
    timer.it_interval.tv_nsec = 0;

    timerfd_settime(timefd, TFD_TIMER_ABSTIME, &timer, NULL);

    return timefd;
}

void * time_fd_init(struct wb_context * ctx){
	struct clock_state * state = mod.data;
	return state;
}
