#include <systemd/sd-bus.h>
#include <unistd.h>
#include <stdlib.h>

#include "module.h"
#include "macro.h"
#include "widget.h"
#include "style.h"

#define CHARGE_PATH "/sys/class/power_supply/AC0/online"
#define POWER_PATH "/sys/class/power_supply/BAT0/capacity"

void get_power_sty(struct wb_config_setting * set, struct wb_style_main * main_sty,
				struct wb_style_base * base);
int get_power_fd(struct wb_context * ctx);
void * power_set(struct wb_context * ctx);
void power_handle(struct wb_event * event, struct wb_context * ctx, void * state);
void handle_power(struct wb_context * ctx, void * state);

struct power_style {
	char * format_full;
	char * format_high;
	char * format_low;
	char * format_empty;
	char * format_charge;
};

struct power_state {
	struct wb_context * ctx;

	int power_level;
	int charge_status;
	char text[64];
	char format[64];
	sd_bus * bus;
};


struct module_interface mod = {		
	.module_name	= "power",
	.parse_sty		= get_power_sty,
	.get_fd			= get_power_fd,
	.set_up			= power_set,
	.handle_event	= power_handle,
	.emit_layout	= handle_power,
	.clean_up		= NULL
};

static const struct config_dispatch dispatch[] = {
	{
		.field_name = "format_full",
		.default_str = (const char *)"{bat}%",
		.field_type = WB_STYLE_STRING,
		.offset = offsetof(struct power_style, format_full)
	},
	{
		.field_name = "format_high",
		.default_str = (const char *)"{bat}%",
		.field_type = WB_STYLE_STRING,
		.offset = offsetof(struct power_style, format_high)
	},
	{
		.field_name = "format_low",
		.default_str = (const char *)"{bat}%",
		.field_type = WB_STYLE_STRING,
		.offset = offsetof(struct power_style, format_low)
	},
	{
		.field_name = "format_empty",
		.default_str = (const char *)"{bat}%",
		.field_type = WB_STYLE_STRING,
		.offset = offsetof(struct power_style, format_empty)
	},
	{
		.field_name = "format_charge",
		.default_str = (const char *)"{bat}%",
		.field_type = WB_STYLE_STRING,
		.offset = offsetof(struct power_style, format_charge)
	}
};

struct module_interface * mod_init(){
	return &mod;
}

void
get_power_sty(struct wb_config_setting * set, struct wb_style_main * msty,
				struct wb_style_base * base)
{
	const struct wb_public_api * api = mod.api;
	struct power_style * style = malloc(sizeof(struct power_style));
	if (style == NULL) {
		LOG_CRIT("Out of memory");
	}

	int setting_length = sizeof(dispatch)/sizeof(dispatch[0]);
	api->config->parse_config(dispatch, setting_length, style, set);
	mod.custom_style = style;
}

static const char *
get_format(struct power_state * state)
{
	const struct power_style * style = mod.custom_style;
	if (state->charge_status) {
		return style->format_charge;
	} else if (state->power_level >= 75) {
		return style->format_full;
	} else if (state->power_level >= 50) {
		return style->format_high;
	} else if (state->power_level >= 10) {
		return style->format_low;
	} else {
		return style->format_empty;
	}
}

static void
draw_text(struct wb_context * ctx, void * data)
{
	struct power_state * state = data;
	const struct wb_public_api * api = mod.api;

	const char * format = get_format(state);
	api->mod->sub_text(format, "bat", state->text,
					&state->power_level, WB_MOD_INT, 64);

	struct wb_widget_text_data text = api->widget->default_text(ctx);
	text.string = state->text;

	api->widget->text(state->ctx, &text);
}

static const struct wb_widget_callback power_cb = {

};

void handle_power(struct wb_context * ctx, void * data){
	struct power_state * state = data;
	const struct wb_public_api * api = mod.api;

	static int id = -1;
	if (id < 0) {
		id = api->widget->allocate_id(ctx);
		api->widget->set_id(ctx, id, state, WB_POINTER_HOVER, &power_cb);
	}

	int event = api->widget->get_event(ctx, id);
	struct wb_widget_rect_special rect = {
		.rect = api->widget->default_rect(ctx, event)
	};

	rect.rect.child_cb = draw_text;
	rect.rect.data = state;

	api->widget->bind_id(ctx, id, &rect);
	api->widget->rect_special(ctx, &rect);
}

void power_handle(struct wb_event * event, struct wb_context * ctx, void * data){
	const struct wb_public_api * api = mod.api;
	struct power_state * state = data;

	while(sd_bus_process(state->bus, NULL) > 0);
}

int power_get(sd_bus_message * m, void * udata, sd_bus_error * ret_error){
	const struct wb_public_api * api = mod.api;
	struct power_state * state = udata;
    
	static int file_fd = 0;

	if (file_fd == 0){
		file_fd = open(POWER_PATH, 0);
	}

    char buffer[16];
    
    int bytereads = read(file_fd, buffer, sizeof(buffer));
    lseek(file_fd, SEEK_SET, 0);
    buffer[bytereads] = 0;
    int battery_now = atoi(buffer);

    if(battery_now == state->power_level)
		return 0;

	state->power_level = battery_now;
	api->mod->trigger_update(state->ctx);
    
    return 0;
}

int ac_get(sd_bus_message * m, void * udata, sd_bus_error * ret_error){
	struct power_state * state = udata;
	const struct wb_public_api * api = mod.api;
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
	api->mod->trigger_update(state->ctx);
    
    return 0;
}

void bat_set(sd_bus * bus, void * data){

	int res = sd_bus_match_signal(bus,
						NULL,
						NULL,
						"/org/freedesktop/UPower/devices/battery_BAT0",
						"org.freedesktop.DBus.Properties",
						"PropertiesChanged",
						power_get,
						data);

	if (res < 0) {
		LOG_ERR("Match signal failed for BAT0\n");
	}
}

void ac_set(sd_bus * bus, void * data){

	int res = sd_bus_match_signal(bus,
						NULL,
						NULL,
						"/org/freedesktop/UPower/devices/line_power_AC0",
						"org.freedesktop.DBus.Properties",
						"PropertiesChanged",
						ac_get,
						data);

	if (res < 0) {
		LOG_ERR("Match signal failed for AC0\n");
	}
}

/*
 * TODO
 * handle NULL state of modules
 */
void * power_set(struct wb_context * ctx){
	struct power_state * state = calloc(1, sizeof(struct power_state));
	if (state == NULL)
			return NULL;

	state->ctx = ctx;
	state->bus = mod.data;
	bat_set(state->bus, state);
	ac_set(state->bus, state);

	power_get(NULL, state, NULL);
	ac_get(NULL, state, NULL);

	return state;
}

int get_power_fd(struct wb_context * ctx){
	sd_bus * bus;
	sd_bus_default_system(&bus);
	mod.data = bus;

	return sd_bus_get_fd(bus);
}
