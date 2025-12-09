#include "fetcher.h"

int get_mem_fd(int it_sec){
    int time_fd = get_time_fd();

    struct itimerspec timer = {0};
    timer.it_value.tv_sec = it_sec;
    timer.it_interval.tv_sec = it_sec;
    set_nonblock(time_fd);

    timerfd_settime(time_fd, 0, &timer, NULL);

    return time_fd;
}

void * mem_get(void * data){
    struct fd_object * object = data;

    int mem_now, mem_used, mem_free;
    char buffer[128], * endptr, drainbuff[128];

    read(object->fd, drainbuff, sizeof(drainbuff));
    
    FILE * mem_fd = fopen(MEM_PATH, "r");

    fgets(buffer,sizeof(buffer),mem_fd);
    
    int space_begin = strcspn(buffer, " ");
    mem_now = strtod(buffer + space_begin, &endptr);

    fgets(buffer, sizeof(buffer), mem_fd);
    fgets(buffer, sizeof(buffer), mem_fd);

    space_begin = strcspn(buffer, " ");
    mem_used = strtod(buffer + space_begin, &endptr);
    
    mem_free = (mem_now - mem_used) * 100 / mem_now;;
    fclose(mem_fd);

    write(object->pipe, &(Event){MEMORY,1, mem_free},sizeof(Event));

}
