#include "style.h"
#include "layout.h"
#include "render.h"
#include "ulist.h"
#include "config.h"

static int
cmp_index(void * node1, void * node2)
{
	int index1 = ((struct wb_layout_node *) node1)->index;
	int index2 = ((struct wb_layout_node *) node2)->index;

	return index2 - index1;
}

struct wb_layout
get_layout(struct wb_style_main * msty)
{
	struct wb_layout layout = {
		.lhead = NULL,
		.chead = NULL,
		.rhead = NULL
	};

	return layout;
}

static int
parse_group(struct wb_config_setting * set)
{
	int group = 0;
	const char * val;
	int res = wb_config_s_lookup(set, "group", &val, WB_STYLE_STRING);

	if(res < 0)
		group = WB_STYLE_GROUP_LEFT;
	else if (strncmp(val, "left", strlen("left")) == 0)
		group = WB_STYLE_GROUP_LEFT;
	else if (strncmp(val, "center", strlen("center")) == 0)
		group = WB_STYLE_GROUP_CENTER;
	else if (strncmp(val, "right", strlen("right")) == 0)
		group = WB_STYLE_GROUP_CENTER;
	else
		group = WB_STYLE_GROUP_LEFT;

	return group;
}

static struct wb_layout_node *
layout_add(struct wb_layout * layout, int group, int index, int id)
{
	struct wb_layout_node * node = malloc(sizeof(struct wb_layout_node));
	node->id = id;
	node->next = NULL;
	node->index = index;

	if (group == WB_STYLE_GROUP_LEFT){
		LL_INSERT_INORDER(layout->lhead, node, cmp_index);
	} 
	else if (group == WB_STYLE_GROUP_CENTER){
		LL_INSERT_INORDER(layout->chead, node, cmp_index);
	}
	else {
		LL_INSERT_INORDER(layout->rhead, node, cmp_index);
	}

	return node;
}

void
parse_layout_hint(struct wb_layout * layout,
		struct wb_config_setting * set, int id)
{
	int group = parse_group(set);
	int index = 0;

	int res = wb_config_s_lookup(set, "index", &index, WB_STYLE_INT);
	if (res < 0)
		index = 0;

	layout_add(layout, group, index, id);
}

