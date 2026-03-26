#ifndef WBRO_BAR
#define WBRO_BAR

struct module_context;
struct wb_render;

void
wb_bar_init(struct wb_render * wrender);

void
wb_bar_render(struct module_context * mod_ctx);

#endif
