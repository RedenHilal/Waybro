#include "../include/displayer.h"

// functions belows (until end of section) are responsible for drawing pixels
// might move it to other function if cairo do some mess

int alc_shm(uint64_t size){
    char name[8];
    name[0] = '/';
    name[7] = 0;
    for (int i = 1;i<6;i++) name[i] = (rand() & 23) + 97;

    int fd = shm_open(name,O_RDWR | O_CREAT | O_EXCL, S_IWUSR | S_IRUSR | S_IWOTH | S_IROTH);
    
    shm_unlink(name);
    ftruncate(fd, size);
    return fd;

}

void draw(struct AppState* appstate){

    cairo_set_source_rgba(appstate->cai_context, 1.0, 105.0/255.0, 196.0/255.0, ALPHA);
    cairo_rectangle(appstate->cai_context,0,0,appstate->width,appstate->height);
    cairo_fill(appstate->cai_context);

    cairo_select_font_face(appstate->cai_context, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(appstate->cai_context,appstate->height);
    
    wl_surface_attach(appstate->surface, appstate->buffer, 0, 0);
    wl_surface_damage_buffer(appstate->surface, 0, 0, appstate->width, appstate->height);
    wl_surface_commit(appstate->surface);
}

void resize(struct AppState* appstate){
    int64_t size = appstate->width * appstate->height * 4;
    int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32,appstate->width);
    int fd = alc_shm(size);

    appstate->buffptr = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    appstate->cai_srfc = cairo_image_surface_create_for_data(appstate->buffptr, CAIRO_FORMAT_ARGB32,
                                                            appstate->width, appstate->height, stride);
    appstate->cai_context = cairo_create(appstate->cai_srfc);

    struct wl_shm_pool * pool = wl_shm_create_pool(appstate->shm, fd, size);
    appstate->buffer = wl_shm_pool_create_buffer(pool, 0, appstate->width, appstate->height, appstate->width * 4, WL_SHM_FORMAT_ARGB8888);
    
    wl_shm_pool_destroy(pool);
    close(fd);
}

// end of drawing section

static void wl_callback_done(void * data, struct wl_callback * callback, uint32_t callback_data){
    struct AppState * appstate = data;
    wl_callback_destroy(callback);
    resize(appstate);
    draw(appstate);
}

const struct wl_callback_listener wl_callback_listener = {
    .done=wl_callback_done
};

static void zwlr_surface_configure(void * data, struct zwlr_layer_surface_v1 * surface,
                                    uint32_t serial,uint32_t width, uint32_t height){
    struct AppState *appstate = data;

    zwlr_layer_surface_v1_ack_configure(surface, serial);

    struct wl_callback * callback = wl_surface_frame(appstate->surface);
    wl_callback_add_listener(callback, &wl_callback_listener, appstate);
    resize(appstate);
    
    draw(appstate);
    wl_display_flush(appstate->display);
}

static void zwlr_surface_close(void * data, struct zwlr_layer_surface_v1 * surface){

}

static const struct zwlr_layer_surface_v1_listener zwlr_surface_listener = {
    .configure = zwlr_surface_configure,
    .closed = zwlr_surface_close
};

void xdg_ping(void*data, struct xdg_wm_base * base, uint32_t serial){
    //struct AppState * appstate = data;
    xdg_wm_base_pong(base,serial);
    
}

static const struct xdg_wm_base_listener wm_base_listener = {
    .ping = xdg_ping,
};

static void geometry(void * data, struct wl_output * output,int32_t x, int32_t y, 
                     int32_t width, int32_t height, int32_t subpixel, const char* make,
                     const char* model, int32_t transform){

}

static void mode(void * data, struct wl_output * wl_output, uint32_t flags,
                 int32_t width, int32_t height, int32_t refresh){
                
}

static void wl_output_done(void *data, struct wl_output* output){

}

static void wl_output_scale(void * data, struct wl_output* output,int32_t factor){

}

static void wl_output_name(void * data, struct wl_output * wl_output, const char * name){

}


static const struct wl_output_listener wl_output_listener={
    .geometry=geometry,
    .mode=mode,
    .done=wl_output_done,
    .scale=wl_output_scale,
    .name=wl_output_name
};

static void wl_registry_global(void * data, struct wl_registry* registry,uint32_t name, const char* interface, uint32_t version){
    //printf("interface: '%s', version: %d, name: %d\n", interface, version, name);
    struct AppState *appstate = data; 

    if (!strcmp(interface, wl_compositor_interface.name)){
        appstate->compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 4);
    }
    else if (!strcmp(interface, wl_shm_interface.name)){
        appstate->shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
    }
    else if (!strcmp(interface, xdg_wm_base_interface.name)){
        appstate->base = wl_registry_bind(registry, name, &xdg_wm_base_interface, 1);
        xdg_wm_base_add_listener(appstate->base, &wm_base_listener, appstate);
    }
    else if (!strcmp(interface, wl_output_interface.name)){
        appstate->output = wl_registry_bind(registry, name, &wl_output_interface, 2);
        wl_output_add_listener(appstate->output, &wl_output_listener, appstate);
    }
    else if (!strcmp(interface, zwlr_layer_shell_v1_interface.name)){
        appstate->zwlr_sh = wl_registry_bind(registry, name, &zwlr_layer_shell_v1_interface, 1);
    }
}

static void wl_registry_global_remove(void* data, struct wl_registry* registry, uint32_t name){

}

int setwayland(struct AppState* appstate){

    // Below are the structs passed onto each listener,
    // except for the xdg_wm_base listener. would make up inconsistensy 
    // in the appstate struct, hence its manually passed at the registry global

    const struct wl_registry_listener listener = {.global = wl_registry_global, .global_remove = wl_registry_global_remove};
    
    appstate->display = wl_display_connect(NULL);
    appstate->registry = wl_display_get_registry(appstate->display);
    
    wl_registry_add_listener(appstate->registry, &listener, appstate);
    wl_display_roundtrip(appstate->display);
    
    appstate->surface = wl_compositor_create_surface(appstate->compositor);
    struct zwlr_layer_surface_v1 * zwlr_surface = zwlr_layer_shell_v1_get_layer_surface(appstate->zwlr_sh, appstate->surface, appstate->output, ZWLR_LAYER_SHELL_V1_LAYER_TOP, "zwlr_layer");
    appstate->zwlr_srfc = zwlr_surface;

    zwlr_layer_surface_v1_set_anchor(zwlr_surface, ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT );
    zwlr_layer_surface_v1_set_size(zwlr_surface, appstate->width, appstate->height);
    zwlr_layer_surface_v1_set_exclusive_zone(zwlr_surface, appstate->height);
    zwlr_layer_surface_v1_add_listener(zwlr_surface, &zwlr_surface_listener, appstate);

 
    
   
    wl_surface_commit(appstate->surface);
    wl_display_roundtrip(appstate->display);


        
    return 0;
}