#include "fetcher.h"

int get_mem_fd(int it_sec);
void mem_get(struct fd_object * object);
void mem_handle(struct epoll_event * event);

struct mem_info {
	u64 mem_cap;
	u64 mem_avail;
	u64 mem_free;
	u64 mem_used;
};

static struct mem_info mem_state;
static struct wb_data mem_data = {
	.data = &mem_state
};

static const struct module_interface mod = {
	.module_name	= "memory",
	.parse_sty		= NULL,
	.get_fd			= get_mem_fd,
	.set_up			= mem_get,
	.handle_event	= mem_handle,
	.handle_update	= NULL,
	.clean_up		= NULL
};

struct module_interface * mod_init(){
	return &mod;
}

int get_mem_fd(int pipe){
    int time_fd = timerfd_create(CLOCK_REALTIME, 0);
	Thread_struct * param = data
	struct mem_style * style = param->styles[MEMORY];
	int it_sec = style->it_sec;

    struct itimerspec timer = {0};
    timer.it_value.tv_sec = it_sec;
    timer.it_interval.tv_sec = it_sec;
    set_nonblock(time_fd);

    timerfd_settime(time_fd, 0, &timer, NULL);

    return time_fd;
}

void mem_handle(struct wb_event * event, struct wb_context * ctx){
	u64 shot;

    read(event->fd, &shot, sizeof(u64));

	mem_get(ctx);
}

void mem_get(struct wb_context * ctx){
    int mem_cap, mem_used, mem_free;
    char buffer[128], * endptr;
    
    FILE * mem_fd = fopen(MEM_PATH, "r");

    fgets(buffer,sizeof(buffer),mem_fd);
    
    int space_begin = strcspn(buffer, " ");
    mem_state.mem_cap = strtod(buffer + space_begin, &endptr);

    fgets(buffer, sizeof(buffer), mem_fd);

    space_begin = strcspn(buffer, " ");
    memstate.mem_free = strtod(buffer + space_begin, &endptr);

    fgets(buffer, sizeof(buffer), mem_fd);

    space_begin = strcspn(buffer, " ");
    memstate.mem_avail = strtod(buffer + space_begin, &endptr);
    
    memstate.mem_used = (mem_cap - mem_free) * 100 / mem_cap;
    fclose(mem_fd);

	wb_send_data(ctx, &mem_data);
}
