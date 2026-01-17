#include <sys/timerfd.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <stdint.h>

#include "module.h"
#include "macro.h"

void clock_parse_sty(struct wb_style_sec * sec, struct wb_style_main * msty); 
int get_time_fd(struct wb_context * ctx);
void time_fd_init(struct wb_context * ctx);
void time_get(struct wb_event * event, struct wb_context * ctx);
void clock_render(struct wb_render * wrender, struct wb_data * data);

struct clock_style {
	struct wb_style_base base;
	char format[WB_STYLE_STR_SIZE_MAX];
};

struct clock_state {
	uint64_t utime;
};

static struct module_interface mod = {
	.module_name	= "clock",
	.parse_sty		= clock_parse_sty,
	.get_fd			= get_time_fd,
	.set_up			= time_fd_init,
	.handle_event	= time_get,
	.handle_update	= clock_render,
	.clean_up		= NULL
};

struct module_interface * mod_init(int id, struct wb_public_api * api){
	mod.id = id;
	mod.api = api;
	mod.data = malloc(sizeof(struct clock_state));

	_Static_assert(sizeof(struct clock_state) <= 
					sizeof(((struct wb_data *)0)->str_val));

	return &mod;
}

void clock_render(struct wb_render * wrender, struct wb_data * data){
	struct wb_public_api * api = mod.api;
	struct clock_state state;
	memcpy(&state, data->str_val, sizeof(struct clock_state));

	int hour = state.utime / 60;
	int minute = state.utime % 60;

	printf("Event Triggered Clock | Clock %0.2d:%0.2d\n", hour, minute);
}

void clock_parse_sty(struct wb_style_sec * sec, struct wb_style_main * msty){
	struct wb_public_api * api = mod.api;

	struct clock_style * sty = calloc(1, sizeof(struct clock_style));	
	char * format = api->style->get_str(sec, "format");
	strncpy(sty->format, format, sizeof(sty->format));
	mod.style = sty;

	api->style->get_base(&sty->base, sec, msty);
}

void time_get(struct wb_event * event, struct wb_context * ctx){
	struct wb_public_api * api = mod.api;
	struct clock_state * state = mod.data;
    uint64_t trigger;

    read(event->fd, &trigger, sizeof(uint64_t));

    state->utime += trigger;
	state->utime %= (60 * 24);

	struct wb_data data;
	data.id = mod.id;

	memcpy(data.str_val, state, sizeof(struct clock_state));
	api->mod->send_data(ctx, &data);
}

int get_time_fd(struct wb_context * ctx){
	struct clock_state * state = mod.data;
    int timefd = timerfd_create(CLOCK_REALTIME, 0);

    struct itimerspec timer;
    time_t date_now = time(NULL);
    struct tm * tmstat = localtime(&date_now);
    
    timer.it_value.tv_sec = date_now + (60 - tmstat->tm_sec);
    timer.it_value.tv_nsec = 0;
    timer.it_interval.tv_sec = 60;
    timer.it_interval.tv_nsec = 0;

    timerfd_settime(timefd, TFD_TIMER_ABSTIME, &timer, NULL);
    int minutes = tmstat->tm_min + tmstat->tm_hour * 60;
	state->utime = minutes;

    return timefd;
}

void time_fd_init(struct wb_context * ctx){
	struct clock_state * state = mod.data;
	struct wb_public_api * api = mod.api;
	struct wb_data data;
	data.id = mod.id;

	memcpy(data.str_val, state, sizeof(struct clock_state));

	api->mod->send_data(ctx, &data);
}
