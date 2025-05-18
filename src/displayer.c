#include "../include/displayer.h"
#include "../include/fetcher.h"

// Array of function to handle events type
// index is matched by its corresponding enum value

static void * (*handler_group[DATA_COUNT])(void*data) = {
    handle_sysclick,
    handle_workspace,
    handle_time,
    handle_brightness,
    handle_volume,
    handle_bluetooth,
    handle_network,
    handle_power,
    handle_mpd
};


static pthread_mutex_t mutex[DATA_COUNT];


static int setepoll(int pipe,struct AppState* appstate){
    struct epoll_event event,wlevent;
    event.data.fd = pipe;
    event.events = EPOLLIN;
    int epfd = epoll_create1(IN_CLOEXEC);

    if (epfd < 0)
        ON_ERR("Epoll init - displayer")
    epoll_ctl(epfd, EPOLL_CTL_ADD, pipe, &event);

    int wfd = wl_display_get_fd(appstate->display);
    wlevent.events = EPOLLIN;
    wlevent.data.fd = wfd;
    epoll_ctl(epfd,EPOLL_CTL_ADD,wfd,&wlevent);

    return epfd;
}

static void mutex_init (pthread_mutex_t * mutexes){
    for (int i = 0;i<DATA_COUNT;i++){
        mutex_init(&mutexes[i]);
    }
}

int main(){
    struct AppState appstate = {0};
   
    appstate.width = 1920;
    appstate.height = 30; 
    
    int epfd, pipes[2];
    struct epoll_event sock_event, events[MAKS_EVENT];
    pthread_t threadID;
    Event dump[MAKS_EVENT];
    
    if (pipe(pipes) != 0) 
        ON_ERR("Pipe - main")
    
    Thread_struct param = {pipes[1],&appstate, mutex};
    
    
    setwayland(&appstate);
    wl_display_dispatch_pending(appstate.display);
    epfd = setepoll(pipes[0],&appstate);
    
    if (pthread_create(&threadID, NULL, mainpoll, &param)!=0)
        ON_ERR("pthread cretae - displayer")
    

    while (1){

        int ready = epoll_wait(epfd, events, MAKS_EVENT,-1);

        for(int i = 0;i<ready;i++){
            if(events[i].data.fd == pipes[0]){
                int byte_recvs = read(pipes[0], &dump[i], sizeof(Event));
                if (byte_recvs < 0) {
                    perror("Err on recv\n");
                    continue;
                }

                dump[i].appState = &appstate;
                handler_group[dump[i].type](&dump[i]);
                
            }
            else if (events[i].data.fd == wl_display_get_fd(appstate.display)) {
                wl_display_dispatch(appstate.display);
            }
        }

        wl_display_flush(appstate.display);
        
    }

    return 0;
}
