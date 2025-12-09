#include "fetcher.h"
#include "style.h"
#include "core.h"

static const int fd_count = 11;

typedef struct {
    int event_type;
    void * (*handler)(void *data);
	char dup;
} EventMap;

static EventMap event_handlers[] = {
    { WORKSPACE,           workspace_get    , 0},
    { TIME,                time_get         , 0},
    { BRIGHTNESS,          brightness_get   , 0},
    { VOLUME,              volume_get       , 0},
    { BLUETOOTH,           bluetooth_get    , 0},
    { NETWORK,             network_get      , 0},
    { BATTERY_STATUS,      sd_bus_handler   , 0},
    { MPD,                 mpd_get          , 0},
    { CHARGE_STATUS,       sd_bus_handler   , 1},
    { MEMORY,              mem_get          , 0},
    { TEMP,                temp_get         , 0}
};


static int set_epoll(int * fds, struct fd_object *object){
    int epfd = epoll_create1(IN_CLOEXEC);
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLET;
    for(int i = 0;i<fd_count;i++){
        if (fds[i] < 0) {
            printf("Error: Invalid fd %d\n", fds[i]);
        }
		if (event_handlers[i].dup) continue;
        event.data.ptr = &object[i];
        object[i].epfd = epfd;
        int res = epoll_ctl(epfd, EPOLL_CTL_ADD, fds[i], &event);
        if(res < 0) {
			printf("ERR: %d\n", i);
			ON_ERR("epoll_ctl - mainpoll")
		}
    }

    return epfd;
}

static void set_handler(int * fds, struct fd_object * object, Thread_struct * param){
    for (int i = 0; i < fd_count;i++){
        char * data = calloc(128,1);
        object[i].pipe = param->pipe;
        object[i].fd = fds[i];
        object[i].handler = event_handlers[i].handler;
        object[i].event = event_handlers[i].event_type;
        object[i].data = data;
        object[i].styles = param->styles[i];
    }
}

void *mainpoll(void * data){

    struct fd_object fd_handler_object[fd_count];
    struct epoll_event events[MAKS_EVENT];
    Thread_struct * param = data; 

    int brightness_fd = get_brightness_fd();
    int net_fd = get_net_fd();
    int mpd_fd = get_mpd_fd();
    int workspace_fd = get_workspace_fd();
    int bluetooth_fd = get_bluetooth_fd();
    int time_fd = get_time_fd();
    int volume_fd = get_volume_fd();
    int power_fd = get_power_fd(&param->pipe);
    int ac_fd = get_ac_fd(&param->pipe);
    int mem_fd = get_mem_fd(((struct mem_style *)param->styles[MEMORY])->it_sec);
    int temp_fd = get_temp_fd(((struct temp_style *)param->styles[TEMP])->it_sec);


    int fds[] = {
        workspace_fd, time_fd, brightness_fd, volume_fd, 
        bluetooth_fd, net_fd, power_fd, mpd_fd, ac_fd,
        mem_fd, temp_fd
    };
    
    set_handler(fds, fd_handler_object,param);
    
    int epfd = set_epoll(fds, fd_handler_object);
    
    get_workspace_data(&fd_handler_object[0]);
    time_fd_init(&fd_handler_object[1]);
    brightness_get(&fd_handler_object[2]);
    get_volume_data(&fd_handler_object[3]);
    get_bluetooth_data(&fd_handler_object[4]);
    net_set(&fd_handler_object[5]);
    power_get(NULL, param, NULL);
    mpd_get(&fd_handler_object[7]);
    ac_get(NULL, param, NULL);
    mem_get(&fd_handler_object[9]);
    temp_get(&fd_handler_object[10]);

    while (1) {
        int ready = epoll_wait(epfd, events, MAKS_EVENT, -1);

        for(int i = 0;i < ready;i++){
            struct fd_object * object_fd = events[i].data.ptr;
            object_fd->handler(object_fd);
        }
    }

}
