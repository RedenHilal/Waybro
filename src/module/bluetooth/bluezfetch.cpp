#include "BlueZState.hpp"

#include <unistd.h>
#include <string>
#include <vector>

#include "module.h"
#include "widget.h"
#include "style.h"

extern "C" {


void parseBzStyle(struct wb_config_setting * set, struct wb_style_main * msty,
				struct wb_style_base * base);

int getBzFd(struct wb_context * ctx);

void * setUpBzState(struct wb_context * ctx);

void handleBzEvent(struct wb_event * event, struct wb_context * ctx, void * data);

void renderBz(struct wb_context * ctx, void * data);

static struct module_interface mod = {
	.module_name	= "bluetooth",
	.parse_sty		= parseBzStyle,
	.get_fd			= getBzFd,
	.set_up			= setUpBzState,
	.handle_event	= handleBzEvent,
	.emit_layout	= renderBz,
	.clean_up		= NULL
};

struct module_interface * mod_init() {
	return &mod;
}

void parseBzStyle(struct wb_config_setting * set, struct wb_style_main * msty,
				struct wb_style_base * base)
{

}

static const struct wb_widget_callback blueZCallback = {
	
};

void drawText(struct wb_context * ctx, void * data)
{
	const struct wb_public_api * api = mod.api;
	BlueZState * state = static_cast<BlueZState *>(data);

	struct wb_widget_text_data text = api->widget->default_text(ctx);
	char * buffer = state->text_.data();

	if (state->isPowered()) {
		int count = state->getConnectedCount();
		api->mod->sub_text(mod.base_style->format, "count", buffer,
						&count, WB_MOD_INT, 64);
	} else {
		api->mod->sub_text(mod.base_style->format, "count", buffer,
						"OFF", WB_MOD_STRING, 64);
	}

	text.string = buffer;

	api->widget->text(ctx, &text);
}

void renderBz(struct wb_context * ctx, void * data)
{
	const struct wb_public_api * api = mod.api;
	BlueZState * state = static_cast<BlueZState *>(data);

	static int id = -1;
	if (id < 0) {
		id = api->widget->allocate_id(ctx);
		api->widget->set_id(ctx, id, state, WB_POINTER_HOVER, &blueZCallback);
	}

	int event = api->widget->get_event(ctx, id);
	struct wb_widget_rect_special rect = {
		.rect = api->widget->default_rect(ctx, event)
	};

	rect.rect.child_cb = drawText;
	rect.rect.data = state;

	api->widget->bind_id(ctx, id, &rect);
	api->widget->rect_special(ctx, &rect);
}

void handleBzEvent(struct wb_event * event, struct wb_context * ctx, void * data)
{
	const struct wb_public_api * api = mod.api;
	BlueZState * state = static_cast<BlueZState *>(data);

	state->processEvent();
	if (state->isStateChanged()) {
		api->mod->trigger_update(ctx);

	}
}

void * setUpBzState(struct wb_context * ctx)
{
	BlueZState * state = static_cast<BlueZState *>(mod.data);
	state->setUp();
	state->getManagedObjects();
	state->subSignals();

	return state;
}

int getBzFd(struct wb_context * ctx)
{
	auto * state = new BlueZState(sdbus::createSystemBusConnection());
	mod.data = state;

	return state->getFd();
}

}	/* extern C */
