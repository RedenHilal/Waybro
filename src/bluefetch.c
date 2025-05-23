#include "../include/fetcher.h"

static pid_t pid;

void * bluetooth_get(void* data){
    struct fd_object * object = data;
    int * bluetooth_count = object->data;

    char buffer[1024];
    Event blueevent;
    int bytereads; 
    
    bytereads = read(object->fd, buffer, sizeof(buffer) - 1) ;
    buffer[bytereads] = 0;
    if (strstr(buffer, "Connected: yes") != NULL) {
        (*bluetooth_count) += 1;
    } else if (strstr(buffer, "Connected: no") != NULL) {
        (*bluetooth_count) -= 1;
    } else {
        return NULL;
    }

    blueevent.specifier = 1;
    blueevent.type = BLUETOOTH;
    blueevent.data = (void*)bluetooth_count;

    write(object->pipe, &blueevent, sizeof(Event));
    

    return NULL;
}

int get_bluetooth_fd(){
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

void get_bluetooth_data(void * data){
    struct fd_object * object = data;
    int * object_data = object->data;
    char buffer[32] = {0};

    FILE * res = popen("bluetoothctl devices Connected | wc -l", "r");

    fgets(buffer,sizeof(buffer) - 1,res);

    int bluetooth_con = atoi(buffer);
    *object_data = bluetooth_con;

    write(object->pipe, &(Event){BLUETOOTH,0,bluetooth_con,object->data}, sizeof(Event));
}