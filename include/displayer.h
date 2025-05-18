#ifndef DISPLAYER_H
#define DISPLAYER_H

#include <stdio.h>
#include <stdint.h>
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

#include <time.h>
#include <signal.h>
#include <errno.h>
#include <assert.h>

#include <sys/stat.h>
#include <sys/mman.h>

#include <cairo/cairo.h>
#include <pango/pango.h>
#include <pango/pangocairo.h>

#include <wayland-client.h>
#include "wlr-layer-shell.h"
#include "xdg-shell-client.h"

#define MAKS_EVENT 64
#define DATA_COUNT 9
#define ALPHA 0.8

#define ON_ERR(trigger) {printf("ERR on: %s\n", trigger); exit(1);}
#define INT_BITS (sizeof(int) * 8)


typedef struct {
    int type; // fill it with enum event
    int specifier;
    int value;
    char * data;

     // appstate is passed from main and should not be overwriten

     struct AppState * appState; 
} Event;

typedef struct {
    int pipe;
    struct AppState * appState;
    pthread_mutex_t * mutex;
    // maybe another data
} Thread_struct;

struct AppState{
    struct wl_display * display;
    struct wl_registry * registry;
    struct wl_surface * surface;
    struct wl_compositor * compositor; 
    struct wl_shm * shm;
    struct wl_buffer * buffer;
    struct wl_output * output;
    struct xdg_wm_base * base;
    struct zwlr_layer_shell_v1 * zwlr_sh;
    struct zwlr_layer_surface_v1 * zwlr_srfc;
    uint8_t * buffptr;
    uint32_t width;
    uint32_t height;
    uint32_t old_width;
    uint32_t old_height;
    cairo_surface_t * cai_srfc;
    cairo_t * cai_context;
    struct panel_data * panel_data;
};

struct fd_object {
    int fd;
    int event;
    void * (*handler)(void * data);
    void * data;
    int pipe;
    int epfd;
    pthread_mutex_t * mutex;
};

struct panel_data {
    int enabled;
    int type;
    int x;
    int y;
    int width;
    int height;
    char  path[5][128];
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
    MPD
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