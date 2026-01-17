#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include <sys/socket.h>
#include <sys/un.h>

#include "module.h"
#include "ulist.h"
#include "macro.h"

#define WS_MAX_COUNT 64

void parse_ws_sty(struct wb_style_sec * sec, struct wb_style_main * msty);
int get_workspace_fd(struct wb_context * ctx);
void workspace_get(struct wb_event * event, struct wb_context * ctx);
void get_workspace_data(struct wb_context * ctx);
void ws_render(struct wb_render * render, struct wb_data * data);

struct ws_style {
    struct wb_style_base base;
    int max_ws;
    int radius;
    int h_color;
};

struct ws_node {
	int ws_id;
	struct ws_node * next, * prev;
};

struct ws_data {
	struct wb_public_api * api;
	struct ws_node * head;
	int a_ws;
};

static struct module_interface mod = {
	.module_name	= "hyprworkspace",
	.parse_sty		= parse_ws_sty,
	.get_fd			= get_workspace_fd,
	.set_up			= get_workspace_data,
	.handle_event	= workspace_get,
	.handle_update	= ws_render,
	.clean_up		= NULL
};

struct module_interface * mod_init(int id, struct wb_public_api * api){
	printf("ws id = %d\n", id);
	mod.id = id;

	struct ws_data * wsdata = malloc(sizeof(struct ws_data));
	_Static_assert(sizeof(struct ws_data) <= 
					sizeof(((struct wb_data *)0)->str_val));

	mod.data = wsdata;
	wsdata->api = api;
	wsdata->head = NULL;

	return &mod;
}

static int insert_cmp(void * ptr1, void * ptr2){
	struct ws_node * node1 = ptr1;
	struct ws_node * node2 = ptr2;

	return node2->ws_id - node1->ws_id;
}

void ws_render(struct wb_render * wrender, struct wb_data * data){
	struct ws_node * head = data->data;
	int a_ws = data->int_val;

	printf("Event Triggered workspace | Active ws  = %d\n", a_ws);
}

void parse_ws_sty(struct wb_style_sec * sec, struct wb_style_main * msty){
	struct ws_data * wsdata = mod.data;
	struct wb_public_api * api = wsdata->api;

	struct ws_style * ws_sty = calloc(1, sizeof(struct ws_style));
	mod.style = ws_sty;

	api->style->get_base(&ws_sty->base, sec, msty);
}

void workspace_get(struct wb_event * event, struct wb_context * ctx){
	struct ws_data * wsdata = mod.data;
	struct wb_public_api * api = wsdata->api;
    
	const char * cmd_create = "createworkspace>>";
	const char * cmd_destroy = "destroyworkspace>>";
	const char * cmd_ws = "workspace>>";

    char buffer[1024] = {0};
    char * iter;

    read(event->fd, buffer, sizeof(buffer));

    if((iter = strstr(buffer, cmd_create))) {
        int created_workspace = atoi(iter + strlen(cmd_create));

		struct ws_node * node = calloc(1, sizeof(struct ws_node));
		node->ws_id = created_workspace;
        
		DL_INSERT_INORDER(wsdata->head, node, insert_cmp);
    }
    else if ((iter = strstr(buffer, cmd_destroy)) ) {
        int destroyed_workspace = atoi(iter + strlen(cmd_destroy));
		struct ws_node * del_node = NULL;

		DL_SEARCH_SCALAR(wsdata->head, del_node, ws_id, destroyed_workspace);
		if (del_node == NULL)
			return;
		
		DL_DELETE(wsdata->head, del_node);
		free(del_node);
    }
    else if((iter = strstr(buffer, cmd_ws))) {
        int workspace_now = atoi(iter + strlen(cmd_ws));
        wsdata->a_ws = workspace_now;
    }

	struct wb_data data = {
		.id = mod.id,
		.int_val = wsdata->a_ws,
		.data = wsdata->head
	};

	api->mod->send_data(ctx, &data);
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

void get_workspace_data(struct wb_context * ctx){
	struct ws_data * wsdata = mod.data;
	struct wb_public_api * api = wsdata->api;

    char buffer[4096];
    int available_ws = 0;

    wsdata->a_ws = get_active_ws();

    memset(buffer, 0, sizeof(buffer));

    FILE * available_ws_fd = popen("hyprctl  workspaces","r");
    fread(buffer, 1, sizeof(buffer) - 1, available_ws_fd);

    char * iter = buffer;

    while((iter = strstr(iter, " ID "))) {
        iter+=4;
        int wsid = atoi(iter);

		struct ws_node * node = calloc(1, sizeof(struct ws_node));
		node->ws_id = wsid;
		DL_INSERT_INORDER(wsdata->head, node, insert_cmp);
		available_ws++;
    }

	struct wb_data data = {
		.id = mod.id,
		.int_val = wsdata->a_ws,
		.data = wsdata->head
	};
    
    pclose(available_ws_fd);
	api->mod->send_data(ctx, &data);
}
