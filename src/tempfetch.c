#include "fetcher.h"

int get_temp_fd(int it_sec){
    int time_fd = get_time_fd();
    set_nonblock(time_fd);

    struct itimerspec timer = {0};
    timer.it_value.tv_sec = it_sec;
    timer.it_interval.tv_sec = it_sec;

    timerfd_settime(time_fd, 0, &timer, NULL);

    return time_fd;
}


void * temp_get(void * data){
    struct fd_object * object = data;

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
