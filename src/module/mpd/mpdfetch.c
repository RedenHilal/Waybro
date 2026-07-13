#include "module.h"
#include "widget.h"
#include "macro.h"
#include "style.h"
#include "poll.h"

#include "mpdfetch.h"

#include <libgen.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/inotify.h>

static struct module_interface mod = {
	.module_name	= "mpd",
	.parse_sty		= parse_mpd_sty,
	.get_fd			= get_mpd_fd,
	.set_up			= mpd_get,
	.handle_event	= handle_mpd_event,
	.emit_layout	= mpd_render,
	.clean_up		= NULL
};

static const struct config_dispatch dispatch[] = {
	{
		.field_name = "server_path",
		.default_str = (const char *)"~/.config/mpd/socket",
		.field_type = WB_STYLE_STRING,
		.offset = offsetof(struct mpd_setting, server_path)
	}
};

static const struct wb_widget_callback callback = {

};

struct module_interface *
mod_init()
{
	return &mod;
}

static void
draw_text(struct wb_context * ctx, void * data)
{
	const struct wb_public_api * api = mod.api;
	struct mpd_info * state = data;

	struct wb_widget_text_data text = api->widget->default_text(ctx);
	if (state->connected) {
		text.string = state->curr_song;
	} else {
		text.string = "MPD down";
	}

	api->widget->text(ctx, &text);
}

void
mpd_render(struct wb_context * ctx, void * data)
{
	const struct wb_public_api * api = mod.api;
	struct mpd_info * state = data;
	static int id = -1;
	
	if (id < 0) {
		id = api->widget->allocate_id(ctx);
		api->widget->set_id(ctx, id, state, WB_POINTER_HOVER, &callback);
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

void
parse_mpd_sty(struct wb_config_setting * set, struct wb_style_main * msty,
				struct wb_style_base * base)
{
	const struct wb_public_api * api = mod.api;
	struct mpd_setting * setting = malloc(sizeof(struct mpd_setting));

	int setting_length = sizeof(dispatch)/sizeof(dispatch[0]);
	api->config->parse_config(dispatch, setting_length, setting, set);

	mod.custom_style = setting;
}

static int
find_starting_utf(char * string, int n)
{
    while(n > 0 && (string[n] & 0xc0) == 0x80){
        n--;
    }
    return n;
}

static void
get_curr_song(struct mpd_info * state)
{
	char * write_buffer = state->curr_song;
	int fd = state->mpd_fd;
    char buffer[1024];

    int bytereads = read(fd, buffer, sizeof(buffer));
	printf("%d %s\n", bytereads, buffer);
	if (bytereads <= 0) {
		state->cmd = MPD_CMD_IDLE;
		return;
	};
    buffer[bytereads] = 0;

    char * title = strstr(buffer, "Title: ");
    char * artist = strstr(buffer, "Artist: ");

    if(!title || !artist) {
        snprintf(write_buffer, MPD_SONG_MAX_LENGTH, "Unknown");
    }
    else {
        title +=7;
        artist += 8;

        char * title_end = strchr(title, '\n');
       char * artist_end = strchr(artist,'\n');
        
        if(title_end) *title_end = 0;
        if(artist_end) *artist_end = 0;
        
        int byte_printed = snprintf(write_buffer, MPD_SONG_MAX_LENGTH,
						"%s - %s", title, artist);
        
        if(byte_printed >= MPD_SONG_MAX_LENGTH){
            int valid_cut = find_starting_utf(write_buffer, MPD_SONG_MAX_LENGTH - 1);
			write_buffer[valid_cut] = 0;
        }
    }

	write(fd, "idle\n", 5);
}

void
handle_idle(struct wb_context * ctx, struct mpd_info * state)
{
    char buffer[512];
    int bytereads = read(state->mpd_fd, buffer, sizeof(buffer));
    
    if(bytereads <= 0) return;
    buffer[bytereads] = 0;

	if (strncmp(buffer, "changed: player", 15) == 0) {
		LOG_INFO("Current song triggered\n");
		state->cmd = MPD_CMD_CURRSONG;
		write(state->mpd_fd, "currentsong\n", 12);
	} else {
		write(state->mpd_fd, "idle\n", 5);
	}

    return;
}

static void
mpd_sock_clean_up(struct wb_event * event, struct wb_context * ctx)
{
	const struct wb_public_api * api = mod.api;
	struct mpd_info * state = event->data;

	api->mod->rmv_sub(ctx, state->handle);
	close(event->fd);
}

static int
connect_socket(struct wb_context * ctx, struct mpd_info * state)
{
	struct mpd_setting * setting = mod.custom_style;
	struct sockaddr_un sock_addr;
	int socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (socket_fd < 0) {
		LOG_ERR("Failed to create unix socket");
		return -1;
	}

	sock_addr.sun_family = AF_UNIX;
	strncpy(sock_addr.sun_path, setting->server_path, sizeof(sock_addr.sun_path) - 1);

	int res = connect(socket_fd, (struct sockaddr*)&sock_addr, sizeof(sock_addr));
	if (res < 0) {
		close(socket_fd);
		return -1;
	}

	return socket_fd;
}

static void
start_socket(struct wb_context * ctx, struct mpd_info * state)
{
	char buffer[512];
	const struct wb_public_api * api = mod.api;
	int socket_fd = connect_socket(ctx, state);
	if (socket_fd < 0) {
		state->connected = 0;
		return;
	}

	struct wb_poll_handle * handle = api->mod->reg_sub(ctx, socket_fd,
					WB_EVENT_READ | WB_EVENT_HUP, state, mod.id);

	read(socket_fd, buffer, sizeof(buffer));
	write(socket_fd, "currentsong\n", 12);
	state->cmd = MPD_CMD_CURRSONG;

	state->handle = handle;
	state->mpd_fd = socket_fd;
	state->connected = 1;
}

void *
mpd_get(struct wb_context * ctx)
{
	const struct wb_public_api * api = mod.api;
    struct sockaddr_un sock_addr;
	struct mpd_info * state = malloc(sizeof(struct mpd_info));
	if (state == NULL) {
		return NULL;
	}

	start_socket(ctx, state);

    return state;
}

static void
handle_sockethup(struct wb_event * event, struct wb_context * ctx)
{
	mpd_sock_clean_up(event, ctx);
}

void
handle_mpd_event(struct wb_event * event, struct wb_context * ctx, void * data)
{
	const struct wb_public_api * api = mod.api;
	struct mpd_info * state = data;

	LOG_INFO("Test MPD\n");
	if (event->fd == state->mpd_fd) {
		if (event->event & WB_EVENT_HUP) {
			state->connected = 0;
			handle_sockethup(event, ctx);
		} else {
			if (state->cmd == MPD_CMD_CURRSONG) {
				get_curr_song(state);
				state->cmd = MPD_CMD_IDLE;
			} else {
				handle_idle(ctx, state);
			}
		}
	} 
	/*
	 * event from inotify
	 * start mpd socket as response
	 */
	else {
		struct inotify_event ievent;
		char buffp[512];
		read(event->fd, buffp, sizeof(buffp));
		start_socket(ctx, state);
	}



	api->mod->trigger_update(ctx);
}

int get_mpd_fd(struct wb_context * ctx){
	const struct mpd_setting * setting = mod.custom_style;
	char dir_path[256];
	strncpy(dir_path, setting->server_path, sizeof(dir_path));

    int intfd = inotify_init1(IN_CLOEXEC);
    if (inotify_add_watch(intfd, dirname(dir_path), IN_CREATE ) < 0)
        ON_ERR("Inotify - mpd")
    
    return intfd;
}

void mpd_fd_init(struct wb_context * ctx){
    

}
