#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/timerfd.h>

#include "widget.h"
#include "module.h"
#include "macro.h"
#include "style.h"

int get_temp_fd(struct wb_context * ctx);
void parse_temp_sty(struct wb_config_setting * set, struct wb_style_main * msty,
				struct wb_style_base * base);
void * set_up_temp(struct wb_context * ctx);
void temp_handle(struct wb_event * event, struct wb_context * ctx, void * data);
void render_temp(struct wb_context * ctx, void * data);

#define TEMP_PATH "/sys/class/thermal/thermal_zone0/temp"

static struct module_interface mod = {
	.module_name	= "temperature",
	.parse_sty		= parse_temp_sty,
	.get_fd			= get_temp_fd,
	.set_up			= set_up_temp,
	.handle_event	= temp_handle,
	.emit_layout	= render_temp,
	.clean_up		= NULL
};

struct temp_state {
	int temp_now;
	int temp_fd;

	char text[64];
};

struct temp_setting {
	int interval;
};

static const struct config_dispatch temp_config[] = {
	{
		.field_name = "interval",
		.default_int = 5,
		.field_type = WB_STYLE_INT,
		.offset = offsetof(struct temp_setting, interval)
	}
};

static const struct wb_widget_callback temp_cb = {

};

struct module_interface * mod_init(){
	return &mod;
}

static void
render_text(struct wb_context * ctx, void * data)
{
	const struct wb_public_api * api = mod.api;
	struct temp_state * state = data;

	struct wb_widget_text_data text = api->widget->default_text(ctx);
	text.string = state->text;

	api->widget->text(ctx, &text);
}

void
render_temp(struct wb_context * ctx, void * data)
{
	const struct wb_public_api * api = mod.api;
	struct temp_state * state = data;
	static int id = -1;

	if (id < 0) {
		id = api->widget->allocate_id(ctx);
		api->widget->set_id(ctx, id, state, WB_POINTER_HOVER, &temp_cb);
	}

	int event = api->widget->get_event(ctx, id);

	struct wb_widget_rect_special rect = {
		.rect = api->widget->default_rect(ctx, event)
	};

	rect.rect.data = state;
	rect.rect.child_cb = render_text;

	api->widget->bind_id(ctx, id, &rect);
	api->widget->rect_special(ctx, &rect);
}

void
parse_temp_sty(struct wb_config_setting * set, struct wb_style_main * msty,
				struct wb_style_base * base)
{
	const struct wb_public_api * api = mod.api;
	struct temp_setting * setting = malloc(sizeof(struct temp_setting));

	int length = sizeof(temp_config)/sizeof(temp_config[0]);
	api->config->parse_config(temp_config, length, setting, set);

	mod.custom_style = setting;
}

int
get_temp_fd(struct wb_context * ctx)
{
	struct temp_setting * setting = mod.custom_style;
	int it_sec = setting->interval;

    int time_fd = timerfd_create(CLOCK_REALTIME, 0);

    struct itimerspec timer = {0};
    timer.it_value.tv_sec = it_sec;
    timer.it_interval.tv_sec = it_sec;

    timerfd_settime(time_fd, 0, &timer, NULL);

    return time_fd;
}

static void
temp_get(struct temp_state * state)
{
	const struct wb_public_api * api = mod.api;
    char buffer[16];

	lseek(state->temp_fd, 0, SEEK_SET);
    read(state->temp_fd, buffer, sizeof(buffer));

    state->temp_now = atoi(buffer) / 1000;
	api->mod->sub_text(mod.base_style->format, "temperature", state->text,
					&state->temp_now, WB_MOD_INT, 64);
}

void *
set_up_temp(struct wb_context * ctx)
{
	struct temp_state * state = malloc(sizeof(struct temp_state));
	struct temp_setting * setting = mod.custom_style;
	int fd = open(TEMP_PATH, O_RDONLY | O_CLOEXEC);

	state->temp_fd = fd;
	temp_get(state);

	return state;
}

void
temp_handle(struct wb_event * event, struct wb_context * ctx, void * data)
{
	const struct wb_public_api * api = mod.api;
	uint64_t count;
	read(event->fd, &count, sizeof(uint64_t));

	temp_get(data);
	api->mod->trigger_update(ctx);
}
