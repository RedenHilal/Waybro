#include <linux/input-event-codes.h>

#include "displayer.h"
#include "style.h"
#include "core.h"
#include "render.h"
#include "macro.h"
#include "widget.h"
#include "comm.h"
#include "bar.h"

struct cb_data {
	struct wb_appstate * appstate;
	struct wb_render * wrender;
	struct wb_context * ctx;
};

int alc_shm(uint64_t size){
    char name[8];
    name[0] = '/';
    name[7] = 0;
    for (int i = 1;i<6;i++) name[i] = (rand() & 23) + 97;

    int fd = shm_open(name,O_RDWR | O_CREAT | O_EXCL, S_IWUSR |
							S_IRUSR | S_IWOTH | S_IROTH);

	if (fd < 0)
		ON_ERR("shared mem alloc failed\n");
    
    shm_unlink(name);
    if (ftruncate(fd, size) < 0)
		ON_ERR("ftruncate\n")
    return fd;

}

void draw(struct wb_appstate* appstate){
    
}

void resize(struct wb_appstate* appstate, struct wb_render * wrender){

    struct wb_style_main * main_sty = wrender->m_style;

    int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32,main_sty->width);
    uint64_t size = main_sty->width * main_sty->height * stride;
    int fd = alc_shm(size);

    appstate->buffptr = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    wrender->cai_srfc = cairo_image_surface_create_for_data(appstate->buffptr, 
					CAIRO_FORMAT_ARGB32, main_sty->width, main_sty->height, stride);

    wrender->cai_context = cairo_create(wrender->cai_srfc);

    struct wl_shm_pool * pool = wl_shm_create_pool(appstate->shm, fd, size);
    appstate->buffer = wl_shm_pool_create_buffer(pool, 0, main_sty->width, 
					main_sty->height, main_sty->width * 4, WL_SHM_FORMAT_ARGB8888);
    
    wl_shm_pool_destroy(pool);
    close(fd);

	wb_bar_init(wrender);
}


static void wl_callback_done(void * data, struct wl_callback * callback, uint32_t callback_data){
    struct cb_data * cb_data = data;
	struct wb_appstate * appstate = cb_data->appstate;
    wl_callback_destroy(callback);
    resize(appstate, cb_data->wrender);
    draw(appstate);
}

const struct wl_callback_listener wl_callback_listener = {
    .done=wl_callback_done
};

static void zwlr_surface_configure(void * data, struct zwlr_layer_surface_v1 * surface,
                                    uint32_t serial,uint32_t width, uint32_t height){
    struct cb_data * cb_data = data;
	struct wb_appstate *appstate = cb_data->appstate;

    zwlr_layer_surface_v1_ack_configure(surface, serial);

    struct wl_callback * callback = wl_surface_frame(appstate->surface);
    wl_callback_add_listener(callback, &wl_callback_listener, cb_data);
    resize(appstate, cb_data->wrender);
    
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
    //struct wb_appstate * appstate = data;
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


static const struct wl_output_listener wl_output_listener = {
    .geometry=geometry,
    .mode=mode,
    .done=wl_output_done,
    .scale=wl_output_scale,
    .name=wl_output_name
};

static void wl_callback_notify_frame(void * data, struct wl_callback * callback,
				uint32_t callback_data);

static const struct wl_callback_listener wl_cb_listen_frame = {
	.done = wl_callback_notify_frame
};

static void wl_callback_notify_frame(void * data, struct wl_callback * callback,
				uint32_t callback_data)
{
	struct wb_context * ctx = data;
	wl_callback_destroy(callback);

	struct wl_callback * next_callback = wl_surface_frame(ctx->appstate->surface);
	wl_callback_add_listener(next_callback, &wl_cb_listen_frame, ctx);

	if (ctx->frame->update || ctx->frame->state_change) {
		ctx->frame->update = 0;
		ctx->frame->state_change = 0;
		wb_bar_trigger_update(ctx);

		wl_surface_commit(ctx->appstate->surface);
	}
	else {
		wl_surface_commit(ctx->appstate->surface);
	}

}

static void
wl_pointer_enter(void * data, struct wl_pointer * pointer, uint32_t serial,
				struct wl_surface * surface, wl_fixed_t x, wl_fixed_t y)
{
	struct cb_data * cb_data = data;
	struct wb_context * ctx = cb_data->ctx;
	struct wb_pointer_state * ptr = ctx->ptr;

	double dx = wl_fixed_to_double(x);
	double dy = wl_fixed_to_double(y);

	ptr->pos.x = dx;
	ptr->pos.y = dy;

}

static void
wl_pointer_leave(void * data, struct wl_pointer * pointer, uint32_t serial,
				struct wl_surface * surface)
{
	struct cb_data * cb_data = data;
	struct wb_context * ctx = cb_data->ctx;
	struct wb_pointer_state * ptr = ctx->ptr;

	ptr->pos.x = -1;
	ptr->pos.y = -1;
	ptr->check = 1;

	LOG_INFO("leave\n");
}

/*
 * TODO
 * state_change field of wb_context are used in 2 different thread
 * hence it shall have lock implementation for thread
 * safety, maybe another solution if there is one
 */
static void
wl_pointer_frame(void * data, struct wl_pointer * pointer)
{
	struct cb_data * cb_data = data;
	struct wb_context * ctx = cb_data->ctx;
	struct wb_pointer_state * ptr = ctx->ptr;
	struct wb_widget_interest_list * ilist = ctx->ilist;
	struct wb_frame_state * frame = ctx->frame;

	if (ctx->frame->update) {
		return;
	}

	if (ptr->check) {
		ptr->check = 0;
		int count = wb_widget_hit_multiple_widget(ctx,
						ptr->pos.x, ptr->pos.y, WB_POINTER_HOVER);

		if (count) {
			frame->update = 1;
		}
	}
	

}

static void
wl_pointer_motion(void * data, struct wl_pointer * pointer, uint32_t time,
				wl_fixed_t x, wl_fixed_t y)
{
	struct cb_data * cb_data = data;
	struct wb_context * ctx = cb_data->ctx;
	struct wb_pointer_state * ptr = ctx->ptr;

	double dx = wl_fixed_to_double(x);
	double dy = wl_fixed_to_double(y);

	ptr->pos.x = dx;
	ptr->pos.y = dy;
	ptr->check = 1;

	//LOG_INFO("wl_pointer_motion - x = %f, y = %f\n", dx, dy);
}

static void
wl_pointer_button(void * data, struct wl_pointer * pointer, uint32_t serial,
				uint32_t time, uint32_t button, uint32_t state)
{
	struct cb_data * cb_data = data;
	struct wb_context * ctx = cb_data->ctx;
	struct wb_pointer_state * ptr = ctx->ptr;

	struct wb_widget_listen_node * node = NULL;

	int widget_id;

	if (button != BTN_LEFT) {
		return;
	}

	widget_id = wb_widget_hit_single_widget(ctx,
					ptr->pos.x, ptr->pos.y, WB_POINTER_BUTTON);

	if (state == 1) {
		ptr->po_id = widget_id;
	}
	else if (state == 0){
		if (ptr->po_id < 0) {
			return;
		}

		if (widget_id == ptr->po_id) {
			node = ctx->ilist->node[widget_id];
			node->events ^= WB_POINTER_BUTTON;
			if (node->on_click) {
				node->on_click(ctx, node->data);
			}
			ptr->check = 1;
		}

	}

}

static void
wl_pointer_axis(void * data, struct wl_pointer * pointer, uint32_t time,
				uint32_t axis, wl_fixed_t value)
{
	
}

static void
wl_pointer_axis_source(void * data, struct wl_pointer * pointer, uint32_t axis_source)
{

}

static void
wl_pointer_axis_stop(void * data, struct wl_pointer * pointer, uint32_t time,
				uint32_t axis){
	
}

static void
wl_pointer_axis_discrete(void * data, struct wl_pointer * pointer, uint32_t axis,
				int discrete)
{

}

static void
wl_pointer_axis_value120(void * data, struct wl_pointer * pointer, uint32_t axis,
				int value120)
{

}

static void
wl_pointer_axis_relative_direction(void * data, struct wl_pointer * pointer,
				uint32_t axis, uint32_t direction)
{

}

static const struct wl_pointer_listener wl_pointer_listener = {
	.enter = wl_pointer_enter,
	.leave = wl_pointer_leave,
	.frame = wl_pointer_frame,
	.motion = wl_pointer_motion,
	.button = wl_pointer_button,
	.axis = wl_pointer_axis,
	.axis_source = wl_pointer_axis_source,
	.axis_stop = wl_pointer_axis_stop,
	.axis_discrete = wl_pointer_axis_discrete,
	.axis_value120 = wl_pointer_axis_value120,
	.axis_relative_direction = wl_pointer_axis_relative_direction
};

static void
wl_seat_capabilitites(void * data, struct wl_seat * seat, uint32_t capability)
{
	struct cb_data * cb_data = data;
	struct wb_appstate * appstate = cb_data->appstate;

	int has_pointer = capability & WL_SEAT_CAPABILITY_POINTER;

	if (appstate->pointer == NULL && has_pointer){
		printf("bind pointer\n");
		appstate->pointer = wl_seat_get_pointer(appstate->seat);
		wl_pointer_add_listener(appstate->pointer, &wl_pointer_listener, cb_data);
	}
	else if (appstate->pointer && !has_pointer){
		printf("free pointer\n");
		wl_pointer_release(appstate->pointer);
		appstate->pointer = NULL;
	}
}

static void
wl_seat_name(void * data, struct wl_seat * seat, const char * name)
{
	
}

static const struct wl_seat_listener wl_seat_listener = {
	.capabilities = wl_seat_capabilitites,
	.name = wl_seat_name
};

static void wl_registry_global(void * data, struct wl_registry* registry,uint32_t name, const char* interface, uint32_t version){
    //printf("interface: '%s', version: %d, name: %d\n", interface, version, name);
	struct cb_data * cb_data = data;
    struct wb_appstate *appstate = cb_data->appstate; 

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
	else if (!strcmp(interface, wl_seat_interface.name)){
		appstate->seat = wl_registry_bind(registry, name, &wl_seat_interface, version);
		wl_seat_add_listener(appstate->seat, &wl_seat_listener, cb_data);
	}
}

static void wl_registry_global_remove(void* data, struct wl_registry* registry, uint32_t name){

}

int setwayland(struct wb_appstate * appstate, struct wb_render * wrender,
				struct wb_context * ctx)
{

	struct cb_data * cb_data = malloc(sizeof(struct cb_data));
	cb_data->appstate = appstate;
	cb_data->wrender = wrender;
	cb_data->ctx = ctx;

    const struct wl_registry_listener listener = {
			.global = wl_registry_global,
			.global_remove = wl_registry_global_remove
	};
    struct wb_style_main * main_sty = wrender->m_style;
    
    appstate->display = wl_display_connect(NULL);
    appstate->registry = wl_display_get_registry(appstate->display);
    
    wl_registry_add_listener(appstate->registry, &listener, cb_data);
    wl_display_roundtrip(appstate->display);
    
    appstate->surface = wl_compositor_create_surface(appstate->compositor);

    struct zwlr_layer_surface_v1 * zwlr_surface = zwlr_layer_shell_v1_get_layer_surface(
					appstate->zwlr_sh, appstate->surface, appstate->output,
					ZWLR_LAYER_SHELL_V1_LAYER_TOP, "zwlr_layer");
    appstate->zwlr_srfc = zwlr_surface;

    zwlr_layer_surface_v1_set_anchor(zwlr_surface, ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | 
				ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT);

    zwlr_layer_surface_v1_set_size(zwlr_surface, main_sty->width, main_sty->height);
    zwlr_layer_surface_v1_set_exclusive_zone(zwlr_surface, main_sty->height);
    zwlr_layer_surface_v1_add_listener(zwlr_surface, &zwlr_surface_listener, cb_data);

	struct wl_region * region = 
			wl_compositor_create_region(ctx->appstate->compositor);

	wl_region_add(region, 0, 0, main_sty->width, main_sty->height);
	wl_surface_set_input_region(appstate->surface, region);
	wl_region_destroy(region);
    wl_surface_commit(appstate->surface);

	struct wl_callback * callback = wl_surface_frame(ctx->appstate->surface);
	wl_callback_add_listener(callback, &wl_cb_listen_frame, ctx);

    wl_display_roundtrip(appstate->display);

  
    return 0;
}
