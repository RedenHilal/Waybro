#include "displayer.h"

#ifndef FETCHER_H
#define FETCHER_H

#define POWER_PATH "/sys/class/power_supply/BAT0/capacity"
#define BRIGHTNESS_PATH "/sys/class/backlight/amdgpu_bl1/brightness"
#define CHARGE_PATH "/sys/class/power_supply/AC0/online"
#define NET_PATH "/sys/class/net/wlp1s0/operstate"

// get fd(s)

// power_fd uses 64 bit as the first 32 bit contain inotify fd that should be polled
// and the next 32 bit shall fd to the open file which could be read from
uint64_t get_power_fd();
uint64_t get_ac_fd();

int get_bluetooth_fd();
int get_mpd_fd();
int get_net_fd();
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

void * power_get(void* data);

void * time_get(void* data);

void * mpd_get(void * data);

void * ac_get(void * data);

// dirty init here

void time_fd_init(void*data);
void power_ac_init(struct fd_object*, int,int);

// main poling

void *mainpoll(void * data);

// wayland mess stored here

int setwayland(struct AppState*);
void draw (struct AppState *);

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

// starter function 

void get_workspace_data(void *);

// clean up function




#endif