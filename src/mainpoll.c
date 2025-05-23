#include "../include/fetcher.h"

static int fd_count = 9;

static int events[] = {
    WORKSPACE, TIME, BRIGHTNESS, VOLUME,
    BLUETOOTH, NETWORK, BATTERY_STATUS,MPD, CHARGE_STATUS
};

static void * (*fds_handler[])(void * data) = {
    workspace_get, time_get, brightness_get, volume_get, 
    bluetooth_get, network_get, power_get, mpd_get, ac_get
};

static int set_epoll(int * fds, struct fd_object *object){
    int epfd = epoll_create1(IN_CLOEXEC);
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLET;
    for(int i = 0;i<fd_count;i++){
        if (fds[i] < 0) {
            printf("Error: Invalid fd %d\n", fds[i]);
        }
        event.data.ptr = &object[i];
        object[i].epfd = epfd;
        int res = epoll_ctl(epfd, EPOLL_CTL_ADD, fds[i], &event);
        if(res < 0) ON_ERR("epoll_ctl - mainpoll")
    }

return epfd;
}

static void set_handler(int * fds, struct fd_object * object, Thread_struct * param){
    for (int i = 0; i < fd_count;i++){
        char * data = calloc(124,1);
        object[i].pipe = param->pipe;
        object[i].fd = fds[i];
        object[i].handler = fds_handler[i];
        object[i].event = events[i];
        object[i].data = data;
        object[i].mutex = &param->mutex[events[i]];
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
    uint64_t power_mask = get_power_fd();
    uint64_t ac_mask = get_ac_fd();

    int ac_fd = 0 | (ac_mask >> 32 );
    int power_fd = 0 | (power_mask >> 32);
    int power_file = power_mask & 0xffffffff;
    int ac_file = ac_mask & 0xffffffff;

    int fds[] = {
        workspace_fd, time_fd, brightness_fd, volume_fd, 
        bluetooth_fd, net_fd, power_fd,mpd_fd, ac_fd
    };
    
    resources_init(param->appState);
    set_handler(fds, fd_handler_object,param);
    power_ac_init(fd_handler_object,power_file,ac_file);
    
    int epfd = set_epoll(fds, fd_handler_object);
    power_get(&fd_handler_object[6]);
    ac_get(&fd_handler_object[8]);
    time_fd_init(&fd_handler_object[1]);
    mpd_get(&fd_handler_object[7]);
    network_get(&fd_handler_object[5]);
    brightness_get(&fd_handler_object[2]);

    get_workspace_data(&fd_handler_object[0]);
    get_volume_data(&fd_handler_object[3]);
    get_bluetooth_data(&fd_handler_object[4]);


    while (1) {
        int ready = epoll_wait(epfd, events, MAKS_EVENT, -1);

        for(int i = 0;i < ready;i++){
            struct fd_object * object_fd = events[i].data.ptr;
            object_fd->handler(object_fd);
        }
    }


}
