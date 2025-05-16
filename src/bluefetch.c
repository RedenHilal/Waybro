#include "../include/fetcher.h"

static void killchild();

static pid_t pid;

void * bluetooth_get(void* data){
    struct fd_object * object = data;
    char buffer[1024];
    Event blueevent;
    int bytereads; 
    
    bytereads = read(object->fd, buffer, sizeof(buffer) - 1) ;
    buffer[bytereads] = 0;
    if (strstr(buffer, "Connected: yes") != NULL) {
        blueevent.value = 1;
    } else if (strstr(buffer, "Connected: no") != NULL) {
        blueevent.value = 0;
    } else {
        return NULL;
    }

    int * last_state = object->data;
    if(blueevent.value == *last_state) return NULL;

    blueevent.specifier = 1;
    blueevent.type = BLUETOOTH;
    write(object->pipe, &blueevent, sizeof(Event));
    

    return NULL;
}

int get_bluetooth_fd(){
    atexit(killchild);
    int pipes[2];
    if(pipe(pipes) != 0) 
        ON_ERR("pipe - bluetooth")

    if ((pid = fork()) < 0) 
        ON_ERR("Fork - bluetooth")

    if (pid == 0){
        close(pipes[0]);
        dup2(pipes[1], STDOUT_FILENO);
        close(pipes[1]);

        execlp("/bin/bluetoothctl", "bluetoothctl", "--monitor",NULL);
        ON_ERR("Exec bluetoothctl - bluetooth");
    }

    return pipes[0];
}

static void killchild(){
    if(pid > 0){
        kill(pid, SIGTERM);
    }
}