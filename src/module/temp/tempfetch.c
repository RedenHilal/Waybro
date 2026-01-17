#include "fetcher.h"

int get_temp_fd(int it_sec);
void temp_get(struct fd_object * object);
void temp_handle(struct epoll_event * event);

static const struct module_interface mod = {
	.module_name	= "temperature",
	.parse_sty		= NULL,
	.get_fd			= get_temp_fd,
	.set_up			= temp_get,
	.handle_event	= temp_handle,
	.hanlde_update	= NULL,
	.clean_up		= NULL
};

struct module_interface * mod_init(){
	return &mod;
}

int get_temp_fd(int pipe){
	Thread_struct * param = data;
	struct temp_style * style = param->styles[TEMP];
	int it_sec = style->it_sec;

    int time_fd = timerfd_create(CLOCK_REALTIME, 0);
    set_nonblock(time_fd);

    struct itimerspec timer = {0};
    timer.it_value.tv_sec = it_sec;
    timer.it_interval.tv_sec = it_sec;

    timerfd_settime(time_fd, 0, &timer, NULL);

    return time_fd;
}

void temp_handle(struct wb_event * event, struct wb_context * ctx){
	struct fd_object * object = event.data.ptr;
	temp_get(ctx);
}

void temp_get(struct wb_context * ctx){
    int temp;
    char buffer[128], drainbuff[128];
    FILE * temp_fd = fopen(TEMP_PATH, "r");

    read(object->fd, drainbuff, sizeof(drainbuff));

    fgets(buffer,sizeof(buffer),temp_fd);
    fseek(temp_fd, 0, SEEK_SET);

    temp = atoi(buffer) / 1000;

    fclose(temp_fd);

    write(object->pipe,&(Event){TEMP,1,temp},sizeof(Event));

}
