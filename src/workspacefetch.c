#include "../include/fetcher.h"

void * workspace_get(void* data){
    
    struct fd_object * object = data;
    int * object_data = object->data;
    char buffer[1024] = {0};
    Event workspaceevent;
    char * iter;

    object_data+=2;

    read(object->fd,buffer,sizeof(buffer));

    if((iter = strstr(buffer, "createworkspace>>"))) {\
        int createdworkspace = atoi(iter + 17);
        object_data[createdworkspace / (sizeof(int) * 8)] |= (1 << createdworkspace % (sizeof(int) * 8) );
        *(object_data-1) += 1;
        
        workspaceevent = (Event){WORKSPACE, CREATE_WORKSPACE,createdworkspace,object->data};
        write(object->pipe, &workspaceevent, sizeof(Event));
    }
    else if ((iter = strstr(buffer, "destroyworkspace>>")) ) {
        int destroyedworkspace = atoi(iter + 18);
        object_data[destroyedworkspace / (sizeof(int) * 8)] ^= (1 << destroyedworkspace % (sizeof(int) * 8) );
        *(object_data - 1) -= 1;

        workspaceevent = (Event){WORKSPACE,DESTROY_WORKSPACE,destroyedworkspace,object->data};
        write(object->pipe,&workspaceevent,sizeof(Event));
    }
    else if((iter = strstr(buffer, "workspace>>"))) {
        int workspacenow = atoi(iter + 11);
        *(object_data - 2) = workspacenow;
        
        workspaceevent = (Event){WORKSPACE, ACTIVE_WORKSPACE,workspacenow,object->data};
        write(object->pipe, &workspaceevent, sizeof(Event));
    }

    return NULL;
}

int get_workspace_fd(){
    char * xdg_path = getenv("XDG_RUNTIME_DIR");
    char * his_path = getenv("HYPRLAND_INSTANCE_SIGNATURE");
    char sockpath[124];
    snprintf(sockpath, sizeof(sockpath) - 1, "%s/hypr/%s/.socket2.sock",xdg_path,his_path);

    int sockfd;
    struct sockaddr_un hypr_sock;
    hypr_sock.sun_family = AF_UNIX;
    memcpy(hypr_sock.sun_path, sockpath, strlen(sockpath));
    if((sockfd = socket(AF_UNIX,SOCK_STREAM,0)) < 0)
        ON_ERR("Socket init - workspace")

    if(connect(sockfd, (struct sockaddr *)&hypr_sock, sizeof(hypr_sock)) < 0)
        ON_ERR("Connect socket - workspace")

    return sockfd;
}

void get_workspace_data(void * data){
    struct fd_object * object = data;
    int * ws_data = object->data;
    char buffer[4096];
    int available_ws = 0;

    FILE * active_ws_fd = popen("hyprctl -j activeworkspace", "r");
    fread(buffer,1,sizeof(buffer) - 1,active_ws_fd);
    char * iter = strstr(buffer,"\"id\": ");
    iter += 6;

    *ws_data = atoi(iter);
    ws_data++;
    pclose(active_ws_fd);

    memset(buffer, 0, sizeof(buffer));

    int * workspace_id = ws_data + 1;
    FILE * available_ws_fd = popen("hyprctl  workspaces","r");
    fread(buffer, 1, sizeof(buffer) - 1, available_ws_fd);

    iter = buffer;

    while((iter = strstr(iter, " ID "))) {
        iter+=4;
        int workspace = atoi(iter);
       
        workspace_id[workspace/ INT_BITS] |= (1 << (workspace % INT_BITS));
        available_ws ++;
    }

    *ws_data = available_ws;
    
    pclose(available_ws_fd);

    write(object->pipe,&(Event){WORKSPACE,INFO_WORKSPACE,0,object->data}, sizeof(Event));
}