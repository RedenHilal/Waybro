#define MPD_SONG_MAX_LENGTH 256
#define MPD_SONG_METADATA_LENGTH 64

#define MPD_SK_PATH_MAX_LENGTH 256
#define MPD_NODE_MAX_LENGTH 128
#define MPD_PORT_MAX_LENGTH 16

enum mpd_next_command {
	MPD_CMD_IDLE,
	MPD_CMD_CURRSONG
};

enum mpd_address_type {
	MPD_ADDRESS_TCP_SOCKET,
	MPD_ADDRESS_UNIX_SOCKET
};

struct mpd_info {
	char title[MPD_SONG_METADATA_LENGTH];
	char artist[MPD_SONG_METADATA_LENGTH];
	char album[MPD_SONG_METADATA_LENGTH];

	char text[MPD_SONG_MAX_LENGTH];
	char curr_song[MPD_SONG_MAX_LENGTH];
	struct wb_poll_handle * handle;

	int mpd_fd;
	int cmd;

	char connected;
	char is_playing;
};

struct mpd_setting {
	const char * server_addr;
	int recon_itval;
	int addr_type;
	union {
		struct {
			char path[MPD_SK_PATH_MAX_LENGTH];
		} unix_domain;
		struct {
			char addr[MPD_NODE_MAX_LENGTH];
			char port[MPD_PORT_MAX_LENGTH];
		} tcp;
	};
};

struct metadata_table {
	const char * key;
	char * data;
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
