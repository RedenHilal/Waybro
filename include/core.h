#ifndef WBRO_CORE
#define WBRO_CORE

#include <stdint.h>

#include <cairo/cairo.h>
#include <pango/pango.h>
#include <pango/pangocairo.h>

#include <wayland-client.h>
#include "wlr-layer-shell.h"
#include "xdg-shell-client.h"


#define u64 uint64_t
#define u32 uint32_t
#define u16 uint16_t
#define u8 uint8_t

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

    cairo_surface_t * cai_srfc;
    cairo_t * cai_context;
    struct m_style * m_style;
};

typedef struct thread_struct{
	int pipe;
	struct AppState * appstate;
	void ** styles;
} Thread_struct;



// config parsing
struct component_entries * read_config(char * path, struct AppState * appState);
struct m_style * translate_mstyle(struct component_entries **);

// wayland mess stored here

int setwayland(struct AppState*);
void draw (struct AppState *);

#endif
