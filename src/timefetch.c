#include "../include/fetcher.h"

// first state sent done in time_fd_init()

void * time_get(void* data){
    struct fd_object * object = data;
    char drainbuff[128];

    read(object->fd,drainbuff,sizeof(drainbuff));

    int * minutes_now = object->data;
    *minutes_now = *minutes_now + 1;

    write(object->pipe, &(Event){TIME,0,*minutes_now}, sizeof(Event));
    return NULL;
}

int get_time_fd(){
    int timefd = timerfd_create(CLOCK_REALTIME, 0);
    return timefd;
}

void time_fd_init(void * data){
    struct fd_object * object = data;
    int timefd = object->fd;

    struct itimerspec timer;
    time_t date_now = time(NULL);
    struct tm * tmstat = localtime(&date_now);
    
    timer.it_value.tv_sec = date_now + (60 - tmstat->tm_sec);
    timer.it_value.tv_nsec = 0;
    timer.it_interval.tv_sec = 60;
    timer.it_interval.tv_nsec = 0;

    timerfd_settime(timefd, TFD_TIMER_ABSTIME, &timer, NULL);
    int minutes = tmstat->tm_min + tmstat->tm_hour * 60;

    int * obj_data = object->data;
    *obj_data = minutes;

    write(object->pipe, &(Event){TIME,0,minutes}, sizeof(Event));
 
}