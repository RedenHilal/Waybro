#ifndef WBRO_BAR
#define WBRO_BAR

struct module_context;
struct wb_render;
struct wb_context;

void
wb_bar_init(struct wb_render * wrender);

void
wb_bar_trigger_update(struct wb_context * ctx);

void
wb_bar_render(struct module_context * mod_ctx);

void
wb_bar_erase(struct wb_render * wrender);

#endif
