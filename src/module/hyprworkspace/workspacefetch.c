#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include <sys/socket.h>
#include <sys/un.h>

#include "module.h"
#include "ulist.h"
#include "macro.h"
#include "style.h"
#include "widget.h"

#define WS_MAX_COUNT 64

void parse_ws_sty(struct wb_config_setting * set, struct wb_style_main * main,
				struct wb_style_base * base);
int get_workspace_fd(struct wb_context * ctx);
void workspace_get(struct wb_event * event, struct wb_context * ctx, void * data);
void * get_workspace_data(struct wb_context * ctx);
void ws_render(struct wb_context * ctx, void * data);

struct ws_style {
    int max_ws;
    int radius;
    int h_color;
};

struct ws_node {
	int ws_id;
	int widget_id;
	char text[64];
	struct ws_node * next, * prev;
};

struct ws_data {
	struct ws_node * head;
	int a_ws;
};

static struct module_interface mod = {
	.module_name	= "hyprworkspace",
	.parse_sty		= parse_ws_sty,
	.get_fd			= get_workspace_fd,
	.set_up			= get_workspace_data,
	.handle_event	= workspace_get,
	.emit_layout	= ws_render,
	.clean_up		= NULL
};

struct module_interface *
mod_init(int id, struct wb_public_api * api)
{
	return &mod;
}

static int insert_cmp(void * ptr1, void * ptr2){
	struct ws_node * node1 = ptr1;
	struct ws_node * node2 = ptr2;

	return node1->ws_id - node2->ws_id;
}

static void
handle_click(struct wb_context * ctx, void * data)
{
	struct ws_node * node = data;

	pid_t pid = fork();
	if (pid == 0) {
		execl("/usr/bin/hyprctl", "hyprctl", "dispatch", "workspace", node->text, NULL);
	} else if (pid < 0) {
		LOG_ERR("error on executing hyprctl workspace: \n");
	}
}

static const struct wb_widget_callback ws_cb = {
	.on_click = handle_click
};

static void
ws_node_render_text_cb(struct wb_context * ctx, void * data)
{
	const struct wb_public_api * api = mod.api;
	struct ws_node * node = data;

	struct wb_widget_text_data text = api->widget->default_text(ctx);
	text.string = node->text;

	api->widget->text(ctx, &text);
}

static void
ws_node_render_cb(struct wb_context * ctx, void * data)
{
	const struct wb_public_api * api = mod.api;
	struct ws_style * style = mod.custom_style;
	struct ws_data * state = data;

	struct ws_node * node = state->head;
	struct wb_widget_rect_special rect;
	while(node != NULL) {
		if (node->widget_id < 0) {
			node->widget_id = api->widget->allocate_id(ctx);
			api->widget->set_id(ctx, node->widget_id, node,
							WB_POINTER_HOVER | WB_POINTER_BUTTON, &ws_cb);
		}

		int event = api->widget->get_event(ctx, node->widget_id);
		event &= ~WB_POINTER_BUTTON;

		rect.rect = api->widget->default_rect(ctx, event);
		rect.rect.child_cb = ws_node_render_text_cb;
		rect.rect.data = node;

		if (state->a_ws == node->ws_id) {
			rect.rect.fill_color = (struct wb_widget_color)
							WB_COLOR_FROM_RGBA(style->h_color);
		}

		api->widget->bind_id(ctx, node->widget_id, &rect);
		api->widget->rect_special(ctx, &rect);
		node = node->next;
	}
}

void
ws_render(struct wb_context * ctx, void * data)
{
	const struct wb_public_api * api = mod.api;
	struct ws_data * state = data;
	struct wb_widget_rect_basic rect = {
		.child_cb = ws_node_render_cb,
		.data = data,

		.sizing_width = WB_WIDGET_FIT,
		.sizing_height = WB_WIDGET_GROW
	};

	api->widget->rect(ctx, &rect);
}

void
parse_ws_sty(struct wb_config_setting * set, struct wb_style_main * msty,
				struct wb_style_base * base)
{
	struct ws_style * ws_sty = calloc(1, sizeof(struct ws_style));

	int dim = (msty->module_active_color & 0xff) / 5;
	ws_sty->h_color = msty->module_active_color - dim;
	mod.custom_style = ws_sty;
}

static void
parse_line(const char * line, struct wb_context * ctx, struct ws_data * state)
{
	const struct wb_public_api * api = mod.api;

	const char * cmd_create = "createworkspace>>";
	const char * cmd_destroy = "destroyworkspace>>";
	const char * cmd_ws = "workspace>>";

	if (strncmp(line, cmd_ws, strlen(cmd_ws)) == 0) {
		int aws = atoi(line + strlen(cmd_ws));
		state->a_ws = aws;
		api->mod->trigger_update(ctx);
	} else if (strncmp(line, cmd_create, strlen(cmd_create)) == 0) {
		int cws = atoi(line + strlen(cmd_create));
		struct ws_node * node = calloc(1, sizeof(struct ws_node));
		node->widget_id = -1;
		node->ws_id = cws;
		snprintf(node->text, 64, "%d", cws);
    	  
		DL_INSERT_INORDER(state->head, node, insert_cmp);
		api->mod->trigger_update(ctx);
	} else if (strncmp(line, cmd_destroy, strlen(cmd_destroy)) == 0) {
		int dws = atoi(line + strlen(cmd_destroy));
		struct ws_node * del_node = NULL;

		DL_SEARCH_SCALAR(state->head, del_node, ws_id, dws);
		if (del_node == NULL)
			return;
		
		api->widget->free_id(ctx, del_node->widget_id);
		DL_DELETE(state->head, del_node);
		free(del_node);
		api->mod->trigger_update(ctx);
	}
}

void
workspace_get(struct wb_event * event, struct wb_context * ctx, void * state)
{
	struct ws_data * wsdata = state;
    char buffer[1024] = {0};
    char * iter;

    read(event->fd, buffer, sizeof(buffer));
	iter = strtok(buffer, "\n");
	parse_line(iter, ctx, state);

	while (iter = strtok(NULL, "\n")) {
		parse_line(iter, ctx, state);
	};

}

int get_workspace_fd(struct wb_context * ctx){
    char * xdg_path = getenv("XDG_RUNTIME_DIR");
    char * his_path = getenv("HYPRLAND_INSTANCE_SIGNATURE");
    char sockpath[124];
    snprintf(sockpath, sizeof(sockpath) - 1, "%s/hypr/%s/.socket2.sock",xdg_path,his_path);

    int sockfd;
    struct sockaddr_un hypr_sock = {0};
    hypr_sock.sun_family = AF_UNIX;
    memcpy(hypr_sock.sun_path, sockpath, strlen(sockpath));

    if((sockfd = socket(AF_UNIX,SOCK_STREAM,0)) < 0)
        ON_ERR("Socket init - workspace")

    if(connect(sockfd, (struct sockaddr *)&hypr_sock, sizeof(hypr_sock)) < 0)
        ON_ERR("Connect socket - workspace")

    return sockfd;
}

static int get_active_ws(){
	char buffer[4096];
	
    FILE * active_ws_fd = popen("hyprctl -j activeworkspace", "r");
    fread(buffer, 1, sizeof(buffer) - 1, active_ws_fd);
    char * iter = strstr(buffer,"\"id\": ");
    iter += 6;

    pclose(active_ws_fd);
	return atoi(iter);
}

void * get_workspace_data(struct wb_context * ctx){
	const struct wb_public_api * api = mod.api;

    char buffer[4096] = {0};
    int available_ws = 0;

	struct ws_data * state = malloc(sizeof(struct ws_data));
	state->head = NULL;
    state->a_ws = get_active_ws();

    FILE * available_ws_fd = popen("hyprctl  workspaces","r");
    fread(buffer, 1, sizeof(buffer) - 1, available_ws_fd);

    char * iter = buffer;

    while((iter = strstr(iter, " ID "))) {
        iter+=4;
        int wsid = atoi(iter);

		struct ws_node * node = calloc(1, sizeof(struct ws_node));
		node->ws_id = wsid;
		snprintf(node->text, 64, "%d", wsid);
		node->widget_id = -1;

		DL_INSERT_INORDER(state->head, node, insert_cmp);
		available_ws++;
    }

    pclose(available_ws_fd);
	return state;
}
