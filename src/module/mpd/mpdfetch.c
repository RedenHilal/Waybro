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
#include <sys/types.h>
#include <sys/un.h>
#include <sys/inotify.h>
#include <sys/timerfd.h>
#include <netdb.h>

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
		.default_str = (const char *)"localhost:6600",
		.field_type = WB_STYLE_STRING,
		.offset = offsetof(struct mpd_setting, server_addr)
	},
	{
		.field_name = "reconnect_interval",
		.default_int = 10,
		.field_type = WB_STYLE_INT,
		.offset = offsetof(struct mpd_setting, recon_itval)
	}
};

static const struct wb_widget_callback callback = {

};

struct module_interface *
mod_init()
{
	return &mod;
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
sub_format(struct mpd_info * state)
{
	const struct wb_public_api * api = mod.api;

	api->mod->sub_text(mod.base_style->format, "title", state->text,
					state->title, WB_MOD_STRING, MPD_SONG_METADATA_LENGTH);

	api->mod->sub_text(state->text, "artist", state->text,
					state->artist, WB_MOD_STRING, MPD_SONG_METADATA_LENGTH);

	api->mod->sub_text(state->text, "album", state->text,
					state->album, WB_MOD_STRING, MPD_SONG_METADATA_LENGTH);

	/*
	 * [63] holds the '\0', hence we check length to MPD_SONG_METADATA_LENGTH - 1
	 * which equal to 63 or [62]
	 */
int length = strlen(state->text);
	if (length >= (MPD_SONG_METADATA_LENGTH - 1)) {
		int valid_cut = find_starting_utf(state->text,
						MPD_SONG_METADATA_LENGTH - 2);
		state->text[valid_cut] = 0;
	}
}

static void
draw_text(struct wb_context * ctx, void * data)
{
	const struct wb_public_api * api = mod.api;
	struct mpd_info * state = data;

	sub_format(state);
	struct wb_widget_text_data text = api->widget->default_text(ctx);
	text.string = state->text;
	api->widget->text(ctx, &text);
}

void
mpd_render(struct wb_context * ctx, void * data)
{

	const struct wb_public_api * api = mod.api;
	struct mpd_info * state = data;
	static int id = -1;

	if (!state->connected) {
		return;
	}
	
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

int
get_address_type(const char * address)
{	
	/* tcp type address */
	if (strchr(address, ':')) {
		return MPD_ADDRESS_TCP_SOCKET;
	}
	/* unix socket address */
	else {
		return MPD_ADDRESS_UNIX_SOCKET;
	}
}

static void
parse_unix_path(struct mpd_setting * setting)
{
	if (setting->server_addr[0] == '~') {
		snprintf(setting->unix_domain.path, MPD_SK_PATH_MAX_LENGTH, "%s%s",
						getenv("HOME"), setting->server_addr + 1);
	} else {
		snprintf(setting->unix_domain.path, MPD_SK_PATH_MAX_LENGTH, "%s",
						setting->server_addr);
	}
}

static void
parse_tcp_addr(struct mpd_setting * setting)
{
	const char * port = strchr(setting->server_addr, ':') + 1;
	int addr_length = (long)port - 1 - (long)setting->server_addr;
	
	strncpy(setting->tcp.addr, setting->server_addr, addr_length);
	strncpy(setting->tcp.port, port, MPD_PORT_MAX_LENGTH);
}

void
parse_mpd_sty(struct wb_config_setting * set, struct wb_style_main * msty,
				struct wb_style_base * base)
{
	const struct wb_public_api * api = mod.api;
	struct mpd_setting * setting = malloc(sizeof(struct mpd_setting));

	int setting_length = sizeof(dispatch)/sizeof(dispatch[0]);
	api->config->parse_config(dispatch, setting_length, setting, set);

	setting->addr_type = get_address_type(setting->server_addr);
	if (setting->addr_type == MPD_ADDRESS_TCP_SOCKET) {
		parse_tcp_addr(setting);
	} else {
		parse_unix_path(setting);
	}

	mod.custom_style = setting;
}

static void
parse_curr_song(char * buffer, struct mpd_info * state)
{
	/*
	 * parse the last key first since we'll put null terminator
	 */
	const struct metadata_table mdtable[] = {
		{.key = "Album: ", .data = state->album},
		{.key = "Title: ", .data = state->title},
		{.key = "Artist: ", .data = state->artist}
	};

	const int length = sizeof(mdtable)/sizeof(mdtable[0]);

	for (int i = 0; i < length; i++) {

		char * start = strstr(buffer, mdtable[i].key);

		if (!start) {
			strncpy(mdtable[i].data, "Unknown", MPD_SONG_METADATA_LENGTH);
			return;
		} 

		start += strlen(mdtable[i].key);
		char * val_end = strchr(start, '\n');
		*val_end = 0;

		char * res = strncpy(mdtable[i].data, start, MPD_SONG_METADATA_LENGTH);
		long length = (long)res - (long)mdtable[i].data;
	}
}

static void
get_curr_song(struct mpd_info * state)
{
	char * write_buffer = state->curr_song;
	int fd = state->mpd_fd;
    char buffer[1024];


    int bytereads = read(fd, buffer, sizeof(buffer));
	if (bytereads <= 0) {
		state->cmd = MPD_CMD_IDLE;
		return;
	};

    buffer[bytereads] = 0;
	parse_curr_song(buffer, state);

	int res = write(fd, "idle\n", 5);
	if (res < 0) {
		LOG_ERR("Error on writing querying current song\n");
	}
}

void
handle_idle(struct wb_context * ctx, struct mpd_info * state)
{
    char buffer[512];
    int bytereads = read(state->mpd_fd, buffer, sizeof(buffer));
    
    if(bytereads <= 0) return;
    buffer[bytereads] = 0;

	if (strncmp(buffer, "changed: player", 15) == 0) {
		state->cmd = MPD_CMD_CURRSONG;
		write(state->mpd_fd, "currentsong\n", 12);
	} else {
		write(state->mpd_fd, "idle\n", 5);
		state->cmd = MPD_CMD_IDLE;
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
connect_unix_socket(struct wb_context * ctx, struct mpd_info * state)
{
	struct mpd_setting * setting = mod.custom_style;
	struct sockaddr_un sock_addr;
	int socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (socket_fd < 0) {
		LOG_ERR("Failed to create unix socket");
		return -1;
	}

	sock_addr.sun_family = AF_UNIX;
	strncpy(sock_addr.sun_path, setting->unix_domain.path, sizeof(sock_addr.sun_path) - 1);

	int res = connect(socket_fd, (struct sockaddr*)&sock_addr, sizeof(sock_addr));
	if (res < 0) {
		close(socket_fd);
		return -1;
	}

	return socket_fd;
}

static int
connect_tcp_socket(struct wb_context * ctx, struct mpd_info * state)
{
	struct mpd_setting * setting = mod.custom_style;
	struct addrinfo * res;
	struct addrinfo hints = {
		.ai_family = AF_UNSPEC,
		.ai_socktype = SOCK_STREAM
	};

	int resp = getaddrinfo(setting->tcp.addr, setting->tcp.port,
					&hints, &res);
	if (resp != 0) {
		perror("what\n");
		LOG_INFO("No connection found\n");
		return -1;
	}

	LOG_INFO("AF_INET = %d, AF_INET6 = %d, ai_family = %d\n", AF_INET, AF_INET6, res->ai_family);

	int socket_fd = socket(res->ai_family, SOCK_STREAM, 0);
	if (socket_fd < 0) {
		LOG_ERR("Failed to create tcp socket");
		return -1;
	}
	resp = connect(socket_fd, res->ai_addr, res->ai_addrlen);
	if (connect < 0) {
		LOG_INFO("Failed to connect to tcp mpd\n");
		close(socket_fd);
		return -1;
	}
	LOG_INFO("Connected to tcp socket\n");

	return socket_fd;
}

static void
start_socket(struct wb_context * ctx, struct mpd_info * state)
{
	struct mpd_setting * setting = mod.custom_style;
	char buffer[512];
	const struct wb_public_api * api = mod.api;
	int socket_fd;

	if (setting->addr_type == MPD_ADDRESS_TCP_SOCKET) {
		socket_fd = connect_tcp_socket(ctx, state);
	} else {
		socket_fd = connect_unix_socket(ctx, state);
	}

	if (socket_fd < 0) {
		state->connected = 0;
		return;
	}

	struct wb_poll_handle * handle = api->mod->reg_sub(ctx, socket_fd,
					WB_EVENT_READ | WB_EVENT_HUP | WB_EVENT_RDHUP, state, mod.id);

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
	const struct mpd_setting * setting = mod.custom_style;
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
handle_sockethup(struct wb_event * event, struct wb_context * ctx,
				struct mpd_info * state)
{
	mpd_sock_clean_up(event, ctx);
	state->connected = 0;
}

void
handle_mpd_event(struct wb_event * event, struct wb_context * ctx, void * data)
{
	const struct mpd_setting * setting = mod.custom_style;
	const struct wb_public_api * api = mod.api;
	struct mpd_info * state = data;

	if (event->fd == state->mpd_fd) {
		if (event->event & (WB_EVENT_HUP | WB_EVENT_RDHUP)) {
			handle_sockethup(event, ctx, state);
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
	 * event from inotify/timer
	 */
	else {
		char buffp[512];
		read(event->fd, buffp, sizeof(buffp));

		/* 
		 * always start socket on unix type socket
		 * but check on tcp type socket as its fired
		 * by timer
		 */
		if (setting->addr_type == MPD_ADDRESS_UNIX_SOCKET || !state->connected) {
			start_socket(ctx, state);
		} else {
			return;
		}
	}

	api->mod->trigger_update(ctx);
}


int get_mpd_fd(struct wb_context * ctx){
	struct mpd_setting * setting = mod.custom_style;
	int fd;

	if (setting->addr_type == MPD_ADDRESS_TCP_SOCKET) {
		fd = timerfd_create(CLOCK_REALTIME, 0);

		int it_val = setting->recon_itval;
		struct itimerspec timer = {0};
		timer.it_interval.tv_sec = it_val;
		timer.it_value.tv_sec = it_val;

		timerfd_settime(fd, 0, &timer, NULL);

	} 
	else {
		char dir_path[256];
		strncpy(dir_path, setting->unix_domain.path, sizeof(dir_path));

    	fd = inotify_init1(IN_CLOEXEC);
    	if (inotify_add_watch(fd, dirname(dir_path), IN_CREATE ) < 0)
    	    ON_ERR("Inotify - mpd")
	}

	return fd;
}
