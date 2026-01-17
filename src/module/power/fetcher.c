#include <systemd/sd-bus.h>
#include <unistd.h>
#include <stdlib.h>

#include "module.h"
#include "macro.h"

#define CHARGE_PATH "/sys/class/power_supply/AC0/online"
#define POWER_PATH "/sys/class/power_supply/BAT0/capacity"

void get_power_sty(struct wb_style_sec * sec, struct wb_style_main * main_sty);
int get_power_fd(struct wb_context * ctx);
void power_set(struct wb_context * ctx);
void power_handle(struct wb_event * event, struct wb_context * ctx);
void handle_power(struct wb_render * render, struct wb_data * data);

struct power_style {
	struct wb_style_base base;
	char format[WB_STYLE_STR_SIZE_MAX];
};

struct power_info {
	int power_level;
	int charge_status;
};

struct power_info power_state;
struct wb_data power_data = {
	.data = &power_state
};

sd_bus * gbus = NULL;

struct module_interface mod = {		
	.module_name	= "power",
	.parse_sty		= get_power_sty,
	.get_fd			= get_power_fd,
	.set_up			= power_set,
	.handle_event	= power_handle,
	.handle_update	= handle_power,
	.clean_up		= NULL
};

struct module_interface * mod_init(int id, struct wb_public_api * api){
	mod.id = id;
	power_data.id = id;
	mod.data = api;
	return &mod;
}

void handle_power(struct wb_render * wrender, struct wb_data * data){

	struct wb_public_api * api = mod.data;

	struct power_info * state = data->data;
    struct power_style * style = mod.style;
    struct wb_style_base * base = &style->base;
	char power[6];

	printf("Event Triggered power | Power Status: %d\n", state->power_level); 
	printf("Event Triggered power | Charge Status: %d\n", state->charge_status);  
    
    snprintf(power, 5, "%d%%", state->power_level);

    int rect_x = base->x + base->rd_left;
    int total_width = base->width + base->rd_right + base->rd_left;
    char * text = api->style->str_by_format(style->format, power);

    api->render->erase_area(wrender, base->x , base->y, total_width, base->height);
    api->render->draw_rect(wrender, base->x, base->y, base->width, base->height);

    api->render->draw_text(wrender, base->x, base->y, text);
    free(text);

    api->render->expose_area(wrender, base->x, base->y, total_width, base->height);
}

void get_power_sty(struct wb_style_sec * sec, struct wb_style_main * main_sty){

	struct wb_public_api * api = mod.data;

    struct power_style * pow_sty = calloc(1, sizeof(struct power_style));

    api->style->get_base(&pow_sty->base, sec, main_sty);
    char * format = api->style->get_str(sec, "format");

    strncpy(pow_sty->format, format, WB_STYLE_STR_SIZE_MAX);
	mod.style = pow_sty;
}

void power_handle(struct wb_event * event, struct wb_context * ctx){
	static sd_bus * bus = NULL;

	if(bus == NULL){
		sd_bus_default_system(&bus);
	}

	while(sd_bus_process(bus, NULL) > 0);
}

void set_nonblock(int fd){
    int flag = fcntl(fd, F_GETFL, 0);
    if (flag < 0)
        ON_ERR("fcntl - nonblock")

    if (fcntl(fd, F_SETFL, flag | O_NONBLOCK) < 0)
        ON_ERR("fcntl 2 - nonblock")

    return;
}

int power_get(sd_bus_message * m, void * data, sd_bus_error * ret_error){
	struct wb_public_api * api = mod.data;
    
	static int file_fd = 0;

	if (file_fd == 0){
		file_fd = open(POWER_PATH, 0);
	}

    char buffer[4];
    
    int bytereads = read(file_fd, buffer, sizeof(buffer));
    lseek(file_fd, SEEK_SET, 0);
    buffer[bytereads] = 0;
    int battery_now = atoi(buffer);

    if(battery_now == power_state.power_level) return 0;
    power_state.power_level = battery_now;
    api->mod->send_data(data, &power_data);
    
    return 0;
}

int ac_get(sd_bus_message * m, void * data, sd_bus_error * ret_error){
	printf("charge triggered\n");
	struct wb_public_api * api = mod.data;
	static int file_fd = 0;

	if (file_fd == 0){
		file_fd = open(CHARGE_PATH, 0);
	}
	
	char buffer[4];
    
    int bytereads = read(file_fd, buffer, sizeof(buffer));
    lseek(file_fd, SEEK_SET, 0);

    int charge_now = buffer[0] - 48;

	if (charge_now == power_state.charge_status){
		return 0;
	}
	
	power_state.charge_status = charge_now;
	api->mod->send_data(data, &power_data); 
    
    return 0;
}

void handle_event(struct epoll_event * event){
	if (gbus == NULL){
		sd_bus_default(&gbus);
	}

	while(sd_bus_process(gbus, NULL) > 0);
}

void bat_set(sd_bus * bus, struct wb_context * ctx){
	sd_bus_slot * slot;

	sd_bus_match_signal(bus,
						&slot,
						NULL,
						"/org/freedesktop/UPower/devices/battery_BAT0",
						NULL,
						"PropertiesChanged",
						power_get,
						ctx);

}

void ac_set(sd_bus * bus, struct wb_context * ctx){
	sd_bus_slot * slot;

	sd_bus_match_signal(bus,
						&slot,
						NULL,
						"/org/freedesktop/UPower/devices/line_power_AC0",
						NULL,
						"PropertiesChanged",
						ac_get,
						ctx);

}

void power_set(struct wb_context * ctx){
	sd_bus * bus;
	sd_bus_default_system(&bus);
	bat_set(bus, ctx);
	ac_set(bus, ctx);
	power_get(NULL, ctx, NULL);
	ac_get(NULL, ctx, NULL);

}

int get_power_fd(struct wb_context * ctx){
	sd_bus * bus;
	sd_bus_default_system(&bus);

	return sd_bus_get_fd(bus);
}
