#include "fetcher.h"

int get_bluetooth_fd(void * data);
void get_bluetooth_data(struct fd_object * object);
void bluetooth_get(struct epoll_event * event);

static const struct module_interface mod = {
	.module_name	= "bluetooth",
	.parse_sty		= NULL
	.get_fd			= get_bluetooth_fd,
	.set_up			= get_bluetooth_data,
	.handle_event	= bluetooth_get,
	.handle_update	= NULL,
	.clean_up		= NULL
};

struct module_interface * mod_init(){
	return &mod;
}

static pid_t pid;

void bluetooth_get(struct wb_event * event, struct wb_context * ctx){
    struct fd_object * object = event.data.ptr;
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

int get_bluetooth_fd(int pipe){
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

    proc_reg(pid);
    return pipes[0];
}

void get_bluetooth_data(struct wb_context * ctx){
    int * object_data = object->data;
    char buffer[32] = {0};

    FILE * res = popen("bluetoothctl devices Connected | wc -l", "r");

    fgets(buffer,sizeof(buffer) - 1,res);

    int bluetooth_con = atoi(buffer);
    *object_data = bluetooth_con;

    write(object->pipe, &(Event){BLUETOOTH,0,bluetooth_con,object->data}, sizeof(Event));
}
