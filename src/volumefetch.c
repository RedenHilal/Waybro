#include "../include/fetcher.h"

static pid_t pid;

void * volume_get(void* data){
    struct fd_object * object = data;
    char drainbuff[512];
    char buffer[256];

    read(object->fd,drainbuff,sizeof(drainbuff)) ;
    if(strncmp(drainbuff+7, "change' on sink ", 15) != 0) return NULL;

    FILE * get_vol = popen("pactl get-sink-volume @DEFAULT_SINK@", "r");

    if(get_vol == NULL) ON_ERR("crash")

    fgets(buffer, sizeof(buffer) - 1, get_vol);
    int startvol = strcspn(buffer, "/") + 3;

    int *last_volume = object->data;
    int volume = atoi(buffer + startvol);
    if(*last_volume == volume) {
        pclose(get_vol);
        return NULL;
    }
    *last_volume = volume;

    write(object->pipe,&(Event){VOLUME,0,volume},sizeof(Event));

    pclose(get_vol);

    return NULL;
}

int get_volume_fd(){
    
    int pipes[2];
    if (pipe(pipes) != 0) ON_ERR("pipes - vol fetch")

    if ((pid = fork()) < 0) ON_ERR("fork - vol fetch")

    // child process
    else if (pid == 0){
        close(pipes[0]);
        dup2(pipes[1], STDOUT_FILENO);
        close(pipes[1]);

        execlp("/bin/pactl", "pactl", "subscribe",NULL);

        ON_ERR("exec - vol fetch")
    }

    proc_reg(pid);

    return pipes[0];
}

void get_volume_data(void * data){
    struct fd_object * object = data;
    int * object_data = object->data;
    char buffer[512] ;

    FILE * res = popen("pactl get-sink-volume @DEFAULT_SINK@", "r");

    if(res == NULL) ON_ERR("crash")

    fgets(buffer, sizeof(buffer) - 1, res);
    int startvol = strcspn(buffer, "/") + 3;

    int *last_volume = object->data;
    int volume = atoi(buffer + startvol);
    *last_volume = volume;

    write(object->pipe,&(Event){VOLUME,0,volume, object->data},sizeof(Event));

    pclose(res);
}