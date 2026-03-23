#ifndef WBRO_CORE
#define WBRO_CORE

#include <stdint.h>

#include <wayland-client.h>
#include "wlr-layer-shell.h"
#include "xdg-shell-client.h"

struct wb_render;
struct module_interface;
struct wb_config_setting;


struct wb_appstate{
    struct wl_display * display;
    struct wl_registry * registry;
    struct wl_surface * surface;
    struct wl_compositor * compositor; 
	struct wl_region * region;
    struct wl_shm * shm;
    struct wl_buffer * buffer;
    struct wl_output * output;
	struct wl_seat * seat;
	struct wl_pointer * pointer;
    struct xdg_wm_base * base;
    struct zwlr_layer_shell_v1 * zwlr_sh;
    struct zwlr_layer_surface_v1 * zwlr_srfc;
    uint8_t * buffptr;
};


// wayland mess stored here

int
setwayland(struct wb_appstate * appstate, struct wb_render * wrender);

void
draw(struct wb_appstate *);

struct module_interface **
load_modules(struct wb_config_setting * mods, int * mod_count,
				struct wb_config_setting ** mod_sets, struct wb_appstate * appstate);

#endif
