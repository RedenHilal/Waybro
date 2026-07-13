#ifndef WBRO_WAYLAND
#define WBRO_WAYLAND

struct wb_context;
struct module_context;
struct wb_render;

void
wb_wl_trigger_frame(struct wb_context * ctx, struct module_context * mod_ctx);

#endif
