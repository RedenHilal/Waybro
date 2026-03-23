#ifndef WBRO_LAYOUT
#define WBRO_LAYOUT

struct wb_style_base;

struct wb_layout_node {
	int id;
	int index;
	struct wb_layout_node * next;
	struct wb_style_base * base;
};

struct wb_layout {
	struct wb_layout_node * lhead;
	struct wb_layout_node * chead;
	struct wb_layout_node * rhead;
};

struct wb_layout
get_layout(struct wb_style_main * msty);

void
parse_layout_hint(struct wb_layout * layout,
				struct wb_config_setting * set, int id);

#endif
