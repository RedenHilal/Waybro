#include <dirent.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/inotify.h>

#include "module.h"
#include "widget.h"
#include "macro.h"

#define BKL_OPTION_PATH "/sys/class/backlight"
#define BKL_BRIGHTNESS_FILE "brightness"
#define BKL_MAX_BRIGHTNESS_FILE "max_brightness"

void backlight_parse_sty(struct wb_config_setting * set, struct wb_style_main * msty,
				struct wb_style_base * base);
int get_brightness_fd(struct wb_context * ctx);
void brightness_get(struct wb_event * event, struct wb_context * ctx, void * state);
void * brightness_trigger(struct wb_context * ctx);
void backlight_render(struct wb_context * ctx, void * state);


struct backlight_state {
	char bkl_path[256];
	char text[64];

	int openfd;
	int bkl_level;
	int max_brightness;
};

struct cb_data {
	struct backlight_state * state;
	struct wb_context * ctx;
};

static struct module_interface mod = {
	.module_name	= "backlight",
	.parse_sty		= backlight_parse_sty,
	.get_fd			= get_brightness_fd,
	.set_up			= brightness_trigger,
	.handle_event	= brightness_get,
	.emit_layout	= backlight_render,
	.clean_up		= NULL
};

struct module_interface * mod_init(int id){
	return &mod;
}

static const struct wb_widget_callback bkl_cb = {

};

static void
render_text(struct wb_context * ctx, void * data)
{
	struct cb_data * cb_data = data;
	struct backlight_state * state = cb_data->state;
	const struct wb_public_api * api = mod.api;

	struct wb_widget_text_data text = api->widget->default_text(cb_data->ctx);
	text.string = state->text;

	api->widget->text(cb_data->ctx, &text);
}

void
backlight_render(struct wb_context * ctx, void * data)
{
	const struct wb_public_api * api = mod.api;
	struct backlight_state * state = data;
	static int id = -1;

	if (id < 0) {
		id = api->widget->allocate_id(ctx);
		api->widget->set_id(ctx, id, state, WB_POINTER_HOVER, &bkl_cb);
	}

	int event = api->widget->get_event(ctx, id);
	struct cb_data cb_data = {state, ctx};
	
	struct wb_widget_rect_special rect = {
		.rect = api->widget->default_rect(ctx, event)
	};

	rect.rect.data = &cb_data;
	rect.rect.child_cb = render_text;

	api->widget->bind_id(ctx, id, &rect);
	api->widget->rect_special(ctx, &rect);
}

void
backlight_parse_sty(struct wb_config_setting * set, struct wb_style_main * msty,
				struct wb_style_base * base)
{
	
}

static int
read_brightness(struct backlight_state * state)
{
	char buffer[64];
    lseek(state->openfd, SEEK_SET, 0);

    int bytereads = read(state->openfd, buffer, sizeof(buffer));
   	buffer[bytereads] = 0;

    int brightness = atoi(buffer);
	int brightness_pctg = (brightness * 100) / state->max_brightness;
	return brightness_pctg;
}

void brightness_get(struct wb_event * event, struct wb_context * ctx, void * data){
	const struct wb_public_api * api = mod.api;
	struct backlight_state * state = data;
    struct inotify_event ievent;
    int bytereads = read(event->fd, &ievent, sizeof(struct inotify_event));

	int brightness = read_brightness(state);
	if (brightness == state->bkl_level) {
		return;
	}

	state->bkl_level = brightness;
	snprintf(state->text, 64, "%d%%", brightness);
	api->mod->trigger_update(ctx);
}

void *
brightness_trigger(struct wb_context * ctx){
    char brightnessbuf[64];
	struct backlight_state * state = mod.data;
    
    int file_monitor = open(state->bkl_path, O_RDONLY | IN_CLOEXEC);

	state->openfd = file_monitor;
	int brightness = read_brightness(state);
	state->bkl_level = brightness;
	snprintf(state->text, 64, "%d%%", brightness);

    return state;
}

static int
get_max_brightness(const char * path)
{
	int fd = open(path, O_RDONLY | IN_CLOEXEC);
	lseek(fd, SEEK_SET, 0);

	char buffer[64];
	int bytereads = read(fd, buffer, sizeof(buffer));
	buffer[bytereads] = 0;

	int max_brightness = atoi(buffer);
	return max_brightness;
}

int get_brightness_fd(struct wb_context * ctx){
	DIR * bkl_option = opendir(BKL_OPTION_PATH);
	struct dirent * entry = NULL;
	struct backlight_state * state = malloc(sizeof(struct backlight_state));
	char mbkl_path[256];
	if (state == NULL) {
		return -1;
	}

	while (entry = readdir(bkl_option)) {
		if (entry->d_type & DT_DIR == 0) {
			continue;
		}
		if (entry->d_name[0] == '.') {
			continue;
		}
		
		snprintf(state->bkl_path, 256, "%s/%s/%s", BKL_OPTION_PATH,
						entry->d_name, BKL_BRIGHTNESS_FILE);
		snprintf(mbkl_path, sizeof(mbkl_path), "%s/%s/%s", BKL_OPTION_PATH,
						entry->d_name, BKL_MAX_BRIGHTNESS_FILE);
		state->max_brightness = get_max_brightness(mbkl_path);
		break;
	}

	if (entry == NULL) {
		LOG_ERR("backlight source not found");
		return -1;
	}

    int inotify_fd = inotify_init1(IN_CLOEXEC);
    if (inotify_fd < 0) {
        perror("Error on Brightness Fetch ");
        exit(1);
    }  

    if (inotify_add_watch(inotify_fd, state->bkl_path, IN_MODIFY) < 0){
		LOG_ERR("%s\n", state->bkl_path);
        perror("Error on Watching power file system ");
        exit(1);
    }
	
	mod.data = state;
    return inotify_fd;
}

