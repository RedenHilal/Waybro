#define MPD_SONG_MAX_LENGTH 256

enum mpd_next_command {
	MPD_CMD_IDLE,
	MPD_CMD_CURRSONG
};

struct mpd_info {
	char curr_song[MPD_SONG_MAX_LENGTH];
	struct wb_poll_handle * handle;

	int mpd_fd;
	int cmd;

	char connected;
	char is_playing;
};

struct mpd_setting {
	char * server_path;
};

void
parse_mpd_sty(struct wb_config_setting * set, struct wb_style_main * msty,
				struct wb_style_base * base);

int
get_mpd_fd(struct wb_context * ctx);

void *
mpd_get(struct wb_context * ctx);

void
handle_mpd_event(struct wb_event * event, struct wb_context * ctx, void * state);

void
mpd_render(struct wb_context * ctx, void * data);
