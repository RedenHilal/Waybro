#include <stdlib.h>
#include <stdio.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include <string.h>

#include "module.h"
#include "widget.h"
#include "macro.h"
#include "style.h"

#define MEM_PATH "/proc/meminfo"

int get_mem_fd(struct wb_context * ctx);
void mem_parse_sty(struct wb_config_setting * set, struct wb_style_main * msty,
				struct wb_style_base * base);
void * mem_get(struct wb_context * ctx);
void mem_handle(struct wb_event * event, struct wb_context * ctx, void * state);
void mem_render(struct wb_context * ctx, void * state);

struct mem_state {
	u64 mem_cap;
	u64 mem_avail;
	u64 mem_free;
	u64 mem_used;

	char text[64];
	int mode;
};


static struct module_interface mod = {
	.module_name	= "memory",
	.parse_sty		= mem_parse_sty,
	.get_fd			= get_mem_fd,
	.set_up			= mem_get,
	.handle_event	= mem_handle,
	.emit_layout	= mem_render,
	.clean_up		= NULL
};

struct module_interface * mod_init(){
	return &mod;
}

static void
mem_on_click(struct wb_context * ctx, void * data)
{
	const struct wb_public_api * api = mod.api;
	struct mem_state * state = data;
}

const struct wb_widget_callback mem_cb = {
	.on_click = mem_on_click
};

static void
draw_text(struct wb_context * ctx, void * data)
{
	const struct wb_public_api * api = mod.api;
	struct mem_state * state = data;

	struct wb_widget_text_data text = api->widget->default_text(ctx);
	api->mod->sub_text(mod.base_style->format, "mem", state->text,
					&state->mem_used, WB_MOD_LL, 64);
	text.string = state->text;

	api->widget->text(ctx, &text);
}

void
mem_render(struct wb_context * ctx, void * data)
{
	const struct wb_public_api * api = mod.api;
	struct mem_state * state = data;

	static int id = -1;
	if (id < 0) {
		id = api->widget->allocate_id(ctx);
		api->widget->set_id(ctx, id, state, WB_POINTER_HOVER | WB_POINTER_BUTTON,
						&mem_cb);
	}
	int events = api->widget->get_event(ctx, id);
	struct wb_widget_rect_special rect = {
		.rect = api->widget->default_rect(ctx, events)
	};

	rect.rect.child_cb = draw_text;
	rect.rect.data = state;

	api->widget->bind_id(ctx, id, &rect);
	api->widget->rect_special(ctx, &rect);
}

void
mem_parse_sty(struct wb_config_setting * set, struct wb_style_main * msty,
				struct wb_style_base * base)
{
	
}

int get_mem_fd(struct wb_context * ctx){
    int time_fd = timerfd_create(CLOCK_REALTIME, 0);

	/*
	 * TODO
	 * parse interval from module setting
	 */
	int it_sec = 10;

    struct itimerspec timer = {0};
    timer.it_value.tv_sec = it_sec;
    timer.it_interval.tv_sec = it_sec;

    timerfd_settime(time_fd, 0, &timer, NULL);

    return time_fd;
}

static int
read_mem(struct mem_state * state)
{
    char buffer[128], * endptr;
    FILE * mem_fd = fopen(MEM_PATH, "r");
	if (mem_fd == NULL) {
		return -1;
	}

    fgets(buffer,sizeof(buffer),mem_fd);
    
    int space_begin = strcspn(buffer, " ");
	int mem_cap = strtod(buffer + space_begin, &endptr);
    state->mem_cap = mem_cap;

    fgets(buffer, sizeof(buffer), mem_fd);

    space_begin = strcspn(buffer, " ");
	int mem_free = strtod(buffer + space_begin, &endptr);
    state->mem_free = mem_free;

    fgets(buffer, sizeof(buffer), mem_fd);

    space_begin = strcspn(buffer, " ");
	int mem_avail = strtod(buffer + space_begin, &endptr);
    state->mem_avail = mem_avail;
    
    state->mem_used = (mem_cap - mem_avail) * 100 / mem_cap;

    fclose(mem_fd);
	return 0;
}

void mem_handle(struct wb_event * event, struct wb_context * ctx, void * data){
	const struct wb_public_api * api = mod.api;
	u64 shot;
    read(event->fd, &shot, sizeof(u64));

	read_mem(data);
	api->mod->trigger_update(ctx);
}

void * mem_get(struct wb_context * ctx){
    int mem_cap, mem_used, mem_free;
	struct mem_state * state = malloc(sizeof(struct mem_state));
    int res = read_mem(state);

	return state;
}
