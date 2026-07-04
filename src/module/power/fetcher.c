#include <systemd/sd-bus.h>
#include <unistd.h>
#include <stdlib.h>

#include "module.h"
#include "macro.h"
#include "widget.h"

#define CHARGE_PATH "/sys/class/power_supply/AC0/online"
#define POWER_PATH "/sys/class/power_supply/BAT0/capacity"

void get_power_sty(struct wb_config_setting * set, struct wb_style_main * main_sty,
				struct wb_style_base * base);
int get_power_fd(struct wb_context * ctx);
void * power_set(struct wb_context * ctx);
void power_handle(struct wb_event * event, struct wb_context * ctx, void * state);
void handle_power(struct wb_context * ctx, void * state);

struct power_data {
	struct wb_context * ctx;
	void * state;
};

struct power_style {
	char * format;
};

struct power_info {
	int power_level;
	int charge_status;
	char text[64];
};

sd_bus * gbus = NULL;

struct module_interface mod = {		
	.module_name	= "power",
	.parse_sty		= get_power_sty,
	.get_fd			= get_power_fd,
	.set_up			= power_set,
	.handle_event	= power_handle,
	.emit_layout	= handle_power,
	.clean_up		= NULL
};

struct module_interface * mod_init(){
	return &mod;
}

void
get_power_sty(struct wb_config_setting * set, struct wb_style_main * msty,
				struct wb_style_base * base)
{
	
}

static void
draw_text(void * udata)
{
	struct power_data * data = udata;
	struct power_info * state = data->state;
	struct wb_public_api * api = mod.api;

	snprintf(state->text, 64, "%d", state->power_level);
	struct wb_widget_text_data text = {
		.string = state->text,
		.text_color = {255, 255, 255, 255},
		.font_size = 12
	};

	api->widget->text(data->ctx, &text);
}

static void
draw_power(void * state)
{
	struct power_data * data = state;
	const struct wb_public_api * api = mod.api;
	struct wb_widget_rect_special rect = {
		.rect = {
			.fill_color = {255,255,255,0},
			.child_cb = draw_text,
			.data = state,
			.sizing_width = WB_WIDGET_FIT,
			.sizing_height = WB_WIDGET_GROW,
			.layout_y = WB_WIDGET_CENTER
		},
		.event = {
			.data = state,
			.events = WB_POINTER_ENTER | WB_POINTER_LEAVE
		}
	};

	api->widget->rect_special(data->ctx, &rect);

}

void handle_power(struct wb_context * ctx, void * state){
	struct power_data * data = state;
	struct power_info * pstate = data->state;
	const struct wb_public_api * api = mod.api;

	draw_power(state);
}

void power_handle(struct wb_event * event, struct wb_context * ctx, void * state){
	static sd_bus * bus = NULL;
	const struct wb_public_api * api = mod.api;

	if(bus == NULL){
		sd_bus_default_system(&bus);
	}

	while(sd_bus_process(bus, NULL) > 0);
	api->mod->trigger_update(ctx);
}

void set_nonblock(int fd){
    int flag = fcntl(fd, F_GETFL, 0);
    if (flag < 0)
        ON_ERR("fcntl - nonblock")

    if (fcntl(fd, F_SETFL, flag | O_NONBLOCK) < 0)
        ON_ERR("fcntl 2 - nonblock")

    return;
}

int power_get(sd_bus_message * m, void * udata, sd_bus_error * ret_error){
	const struct wb_public_api * api = mod.data;
	struct power_data * data = udata;
	struct power_info * state = data->state;
    
	static int file_fd = 0;

	if (file_fd == 0){
		file_fd = open(POWER_PATH, 0);
	}

    char buffer[16];
    
    int bytereads = read(file_fd, buffer, sizeof(buffer));
    lseek(file_fd, SEEK_SET, 0);
    buffer[bytereads] = 0;
    int battery_now = atoi(buffer);

    if(battery_now == state->power_level) return 0;
	state->power_level = battery_now;
    
    return 0;
}

int ac_get(sd_bus_message * m, void * udata, sd_bus_error * ret_error){
	printf("charge triggered\n");
	struct power_data * data = udata;
	struct power_info * state = data->state;
	const struct wb_public_api * api = mod.data;
	static int file_fd = 0;

	if (file_fd == 0){
		file_fd = open(CHARGE_PATH, 0);
	}
	
	char buffer[4];
    
    int bytereads = read(file_fd, buffer, sizeof(buffer));
    lseek(file_fd, SEEK_SET, 0);

    int charge_now = buffer[0] - 48;

	if (charge_now == state->charge_status){
		return 0;
	}
	
	state->charge_status = charge_now;
    
    return 0;
}

void handle_event(struct epoll_event * event){
	if (gbus == NULL){
		sd_bus_default(&gbus);
	}

	while(sd_bus_process(gbus, NULL) > 0);
}

void bat_set(sd_bus * bus, void * data){
	sd_bus_slot * slot;

	sd_bus_match_signal(bus,
						&slot,
						NULL,
						"/org/freedesktop/UPower/devices/battery_BAT0",
						NULL,
						"PropertiesChanged",
						power_get,
						data);

}

void ac_set(sd_bus * bus, void * data){
	sd_bus_slot * slot;

	sd_bus_match_signal(bus,
						&slot,
						NULL,
						"/org/freedesktop/UPower/devices/line_power_AC0",
						NULL,
						"PropertiesChanged",
						ac_get,
						data);

}

/*
 * TODO
 * handle NULL state of modules
 */
void * power_set(struct wb_context * ctx){
	struct power_info * state = malloc(sizeof(struct power_info));
	if (state == NULL)
			return NULL;

	struct power_data * data = malloc(sizeof(struct power_data));
	if (data == NULL){
		free(state);
		return NULL;
	}

	data->ctx = ctx;
	data->state = state;

	sd_bus * bus;
	sd_bus_default_system(&bus);
	bat_set(bus, data);
	ac_set(bus, data);

	power_get(NULL, data, NULL);
	ac_get(NULL, data, NULL);

	return data;
}

int get_power_fd(struct wb_context * ctx){
	sd_bus * bus;
	sd_bus_default_system(&bus);

	return sd_bus_get_fd(bus);
}
