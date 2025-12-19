#ifndef DISPLAYER_H
#define DISPLAYER_H

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>

#include <sys/epoll.h>
#include <sys/inotify.h>
#include <sys/timerfd.h>

#include <fcntl.h>
#include <pthread.h>
#include <systemd/sd-bus.h>

#include <time.h>
#include <signal.h>
#include <errno.h>
#include <assert.h>

#include <sys/stat.h>
#include <sys/mman.h>


#define MAKS_EVENT 64
#define DATA_COUNT 11
#define ALPHA 0.8


#define TO_DOUBLE(num) (double)num
#define TO_RGB_FMT(num) TO_DOUBLE(num)/255.0
#define INV_RGB(num) 255.0-TO_DOUBLE(num)
#define TO_ALPHA(num) TO_DOUBLE(num)/100.0

#define ON_ERR(trigger) {printf("ERR on: %s\n", trigger); perror(trigger); exit(1);}
#define INT_BITS (sizeof(int) * 8)

#define PASSABLE_EVENT_SIZE offsetof(Event, appState)


typedef struct {
    int type; // fill it with enum event
    int specifier;
    int value;
    char * data;

     // appstate is passed from main and should not be overwriten
     // therefore the usage of PASSABLE_EVENT_SIZE constant

	struct AppState * appstate;
	void * styles;

} Event;

struct fd_object {
    int fd;
    int event;
    void * (*handler)(void * data);
    void * data;
    int pipe;
    int epfd;
    void * styles;
    pthread_mutex_t * mutex;
};

struct proc_register {
    int proc[DATA_COUNT];
    int count;
};

// enum for event
enum {
    SYS_CLICK,
    WORKSPACE,
    TIME,
    BRIGHTNESS,
    VOLUME,
    BLUETOOTH,
    NETWORK,
    POWER,
    MPD,
    MEMORY,
    TEMP
};

// specifier for event
enum {
    BATTERY_STATUS,
    CHARGE_STATUS
};

enum {
    ACTIVE_WORKSPACE,
    CREATE_WORKSPACE,
    DESTROY_WORKSPACE,
    INFO_WORKSPACE
};

enum {
    MPD_UP,
    MPD_DOWN,
    MPD_SENT,
    MPD_ERR
};


#endif
