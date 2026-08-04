#include "PowerStatus.h"

#include "module.h"
#include "widget.h"
#include "style.h"

extern "C" {

void get_power_sty(struct wb_config_setting * set, struct wb_style_main * msty,
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

static struct module_interface mod = {		
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
		.offset = offsetof(struct power_style, format_full),
		.field_type = WB_STYLE_STRING
	},
	{
		.field_name = "format_high",
		.default_str = (const char *)"{bat}%",
		.offset = offsetof(struct power_style, format_high),
		.field_type = WB_STYLE_STRING
	},
	{
		.field_name = "format_low",
		.default_str = (const char *)"{bat}%",
		.offset = offsetof(struct power_style, format_low),
		.field_type = WB_STYLE_STRING
	},
	{
		.field_name = "format_empty",
		.default_str = (const char *)"{bat}%",
		.offset = offsetof(struct power_style, format_empty),
		.field_type = WB_STYLE_STRING
	},
	{
		.field_name = "format_charge",
		.default_str = (const char *)"{bat}%",
		.offset = offsetof(struct power_style, format_charge),
		.field_type = WB_STYLE_STRING
	}
};

struct module_interface * mod_init(){
	return &mod;
}

static const char *
get_format(PowerStatus * state)
{
	const struct power_style * style = static_cast<power_style *>(mod.custom_style);
	if (state->isCharging) {
		return style->format_charge;
	} else if (state->percentage >= 75) {
		return style->format_full;
	} else if (state->percentage >= 50) {
		return style->format_high;
	} else if (state->percentage >= 10) {
		return style->format_low;
	} else {
		return style->format_empty;
	}
}

void
get_power_sty(struct wb_config_setting * set, struct wb_style_main * msty,
				struct wb_style_base * base)
{
	const struct wb_public_api * api = mod.api;
	power_style * style = new power_style();

	int setting_length = sizeof(dispatch)/sizeof(dispatch[0]);
	api->config->parse_config(dispatch, setting_length, style, set);
	mod.custom_style = style;
}

static void
draw_text(struct wb_context * ctx, void * data)
{
	PowerStatus * state = static_cast<PowerStatus *>(data);
	const struct wb_public_api * api = mod.api;
	const char * format = get_format(state);
	int percentage = state->percentage;
	api->mod->sub_text(format, "bat", state->text.data(),
					static_cast<void*>(&percentage), WB_MOD_INT, 64);

	struct wb_widget_text_data text = api->widget->default_text(ctx);
	text.string = state->text.data();

	api->widget->text(ctx, &text);
}

static const struct wb_widget_callback power_cb = {

};

void handle_power(struct wb_context * ctx, void * state){
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

void power_handle(struct wb_event * event, struct wb_context * ctx, void * data)
{
	const struct wb_public_api * api = mod.api;
	PowerStatus * state = static_cast<PowerStatus *>(data);

	state->processEvents();
	if (state->shouldUpdate()) {
		api->mod->trigger_update(ctx);
	}
}

void * power_set(struct wb_context * ctx){
	PowerStatus * state = static_cast<PowerStatus *>(mod.data);
	state->setUp();

	return state;
}

int get_power_fd(struct wb_context * ctx){
	auto * state = new PowerStatus(sdbus::createSystemBusConnection());
	mod.data = state;

	return state->getFd();
}

}
