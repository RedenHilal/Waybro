#include "fetcher.h"

int get_brightness_fd(void * data);
void brightness_get(struct epoll_event * event)
void brightness_trigger(struct fd_object * object);

static struct wb_data blk_data;

static const struct module_interface mod = {
	.module_name	= "brightness",
	.parse_sty		= NULL,
	.get_fd			= get_brightness_fd,
	.set_up			= brightness_trigger,
	.handle_event	= brightness_get,
	.handle_update	= NULL,
	.clean_up		= NULL
};

struct module_interface * module_init(int id){
	mod.id = id;
	blk_data = id;
	return &mod;
}

void brightness_get(struct wb_event * event, struct wb_context * ctx){
    struct inotify_event ievent;
    bytereads = read(object->fd, &buffer, sizeof(struct inotify_event));
	brightness_trigger(ctx);
}

void brightness_trigger(struct wb_context * ctx){
    char brightnessbuf[124];
    
    int file_monitor = open(BRIGHTNESS_PATH, O_RDONLY | IN_CLOEXEC);
    lseek(file_monitor, SEEK_SET, 0);

    int bytereads = read(file_monitor, brightnessbuf, sizeof(brightnessbuf));
    brightnessbuf[bytereads] = 0;

    int brightness = atoi(brightnessbuf);
	int brightness_pctg = (brightness * 100 + BRIGHTNESS_SCALE / 2) / BRIGHTNESS_SCALE;

	blk_data.qdata.int_val = brightness_pctg;

    wb_send_data(ctx, &blk_data);

    close(file_monitor);
    return NULL;
}

int get_brightness_fd(void * data){
    int inotify_fd = inotify_init1(IN_CLOEXEC);
    if (inotify_fd < 0) {
        perror("Error on Brightness Fetch ");
        exit(1);
    }  

    if (inotify_add_watch(inotify_fd, BRIGHTNESS_PATH, IN_MODIFY) < 0){
        perror("Error on Watching power file system ");
        exit(1);
    }

    set_nonblock(inotify_fd);

    return inotify_fd;
}

