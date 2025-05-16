#include "../include/fetcher.h"

void * sysclick_get(void* data){
    int inotify_fd = inotify_init1(IN_CLOEXEC);
    if (inotify_fd < 0) {
        perror("Error on Click Fetch ");
        exit(1);
    }

    return NULL;
}

void * power_get(void* data){
    struct fd_object * object = data;
    int charge;
    char drainbuff[512];
    char buffer[4];
    
    read(object->fd,drainbuff,sizeof(drainbuff)) ;

    int file_monitor = *(int *) object->data;
    int *last_power =  object->data;
    last_power++;

    int bytereads = read(file_monitor,buffer,sizeof(buffer));
    lseek(file_monitor, SEEK_SET, 0);
    buffer[bytereads] = 0;
    charge = atoi(buffer);

    if(*last_power == charge) return NULL;
    *last_power = charge;
    write(object->pipe, &(Event){POWER,BATTERY_STATUS,charge}, sizeof(Event));
    
    return NULL;
}

void * ac_get(void * data){
    struct fd_object * object = data;
    int charge;
    char drainbuff[512];
    char buffer[4];
    
    read(object->fd,drainbuff,sizeof(drainbuff)) ;

    int charge_monitor = *(int *) object->data;
    int bytereads = read(charge_monitor,buffer,sizeof(buffer));
    lseek(charge_monitor, SEEK_SET, 0);
    charge = buffer[0] - 48;
    write(object->pipe, &(Event){POWER,CHARGE_STATUS,charge}, sizeof(Event));
    
    return NULL;
}


uint64_t get_power_fd(){
    int inotify_fd = inotify_init1(IN_CLOEXEC);
   
    if (inotify_fd < 0) 
        ON_ERR("BS inot - power")

    if (inotify_add_watch(inotify_fd, POWER_PATH, IN_CLOSE_NOWRITE) < 0) 
        ON_ERR("Add watch - batt file")
   
    int file_fd = open(POWER_PATH, O_RDONLY | IN_CLOEXEC);

    uint64_t mask = 0;
    mask = mask | inotify_fd;
    mask = mask << 32;
    mask = mask | file_fd;
    return mask;
}

uint64_t get_ac_fd(){
    int ac_inot = inotify_init1(IN_CLOEXEC);
    if (ac_inot < 0)
        ON_ERR("AC inot - power")
    if(inotify_add_watch(ac_inot,CHARGE_PATH , IN_CLOSE_NOWRITE) < 0) 
    ON_ERR("Add watch - Charge file")

    int file_fd = open(CHARGE_PATH, O_RDONLY | IN_CLOEXEC);

    uint64_t mask = 0;
    mask = ac_inot;
    mask = mask << 32;
    mask = mask | file_fd;
    
    return mask;
}

void power_ac_init(struct fd_object * object, int power_file, int ac_file){
    int * power_file_fd = object[6].data;
    *power_file_fd = power_file;

    int * ac_file_fd = object[8].data;
    *ac_file_fd = ac_file;
    return;
}