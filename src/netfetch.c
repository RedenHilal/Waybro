#include "../include/fetcher.h"

static pid_t pid;

void * network_get(void* data){
    struct fd_object * object = data;
    int net_fd = open(NET_PATH,O_RDONLY | O_CLOEXEC);
    lseek(net_fd, SEEK_SET, 0);
    int * prev_stat = object->data;

    char drainbuff[256];
    char buffer[16];

    read(object->fd,drainbuff,sizeof(drainbuff)) ;
    int bytereads = read(net_fd,buffer,sizeof(buffer));
    buffer[bytereads] = 0;
    int net = 0;

    net = !strncmp(buffer, "up", 2);
    if(net == *prev_stat) {
        close(net_fd);
        return NULL;
    }

    *prev_stat = net;
    write(object->pipe, &(Event){NETWORK,0,net}, sizeof(Event));
    
    close(net_fd);

    return NULL;
}

int get_net_fd(){

    int net = 0,prev = 0;
    int pipes[2];

    if(pipe(pipes) < 0) 
        ON_ERR("pipe - net")

    pid = fork();
    if(pid < 0)
        ON_ERR("fork - net")

    if(pid == 0) {
        close(pipes[0]);
        dup2(pipes[1], STDOUT_FILENO);
        close(pipes[1]);

        execlp("/bin/ip", "ip", "monitor", "link",NULL);
        ON_ERR("exec ip monitor - net")
    }

    set_nonblock(pipes[0]);

    return pipes[0];
}
