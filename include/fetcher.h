#include "displayer.h"

#ifndef FETCHER_H
#define FETCHER_H

#define POWER_PATH "/sys/class/power_supply/BAT0/capacity"
#define CHARGE_PATH "/sys/class/power_supply/AC0/online"
#define NET_PATH "/sys/class/net/wlp1s0/operstate"

#define MEM_PATH "/proc/meminfo"

#ifndef TEMP_PATH
#define TEMP_PATH "/sys/class/thermal/thermal_zone0/temp"
#endif

// get fd(s)

int get_power_fd(void * data);
int get_ac_fd(void * data);
int get_mem_fd(int it_sec);
int get_temp_fd(int it_sec);

int get_bluetooth_fd();
int get_mpd_fd();
int get_net_fd();
void net_set(struct fd_object *);
int get_time_fd();
int get_volume_fd();
int get_workspace_fd();
int get_brightness_fd();

// fetcher function

void * sysclick_get(void* data);

void * workspace_get(void* data);

void * brightness_get(void* data);

void * volume_get(void* data);

void * bluetooth_get(void* data);

void * network_get(void* data);

void * time_get(void* data);

void * mpd_get(void * data);

void * mem_get(void * data);

void * temp_get(void * data);

void * sd_bus_handler(void * data);

int ac_get(sd_bus_message * m, void * data, sd_bus_error * err);
int power_get(sd_bus_message * m, void * data, sd_bus_error * err);

// dirty init here

void time_fd_init(void*data);
void power_ac_init(struct fd_object*, int,int);

// main poling

void *mainpoll(void * data);


// handler function

void * handle_sysclick(void*);
void * handle_workspace(void*);
void * handle_brightness(void*);
void * handle_volume(void*);
void * handle_bluetooth(void*);
void * handle_network(void*);
void * handle_power(void*);
void * handle_time(void*);
void * handle_mpd(void*);
void * handle_idle(void*);
void * handle_mem(void*);
void * handle_temp(void*);

// starter function 

void get_workspace_data(void *);
void get_bluetooth_data(void *);
void get_volume_data(void *);

// clean up function


// misc
void resources_init(void *);
void set_nonblock(int fd);
void handle_segv(int);
void handle_sigttou(int);
void proc_reg(int);


#endif
