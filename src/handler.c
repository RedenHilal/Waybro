#include "../include/fetcher.h"


void * handle_sysclick(void * data){
    Event event = *(Event *) data;
    printf("Event Triggered %d\n", event.type);

    return NULL;
}

void * handle_workspace(void * data){
    Event event = *(Event *) data;
    struct AppState * appState = event.appState;
    int * workspace_data = (int*)event.data;
    int active_workspace = *workspace_data;
    int available_workspace = *(workspace_data+1);
    int workspaces[available_workspace];

    int count = 0;
    int * workspaces_bits = workspace_data + 2; 
    for(int i = 1;count < available_workspace;i++){
        if(workspaces_bits[i / INT_BITS] & (1 << (i % INT_BITS))){
            workspaces[count] = i;
            count++;
        }
    }

    switch (event.specifier) {
        case ACTIVE_WORKSPACE:
            printf("Event Triggered %d | Active workspace: %d\n", event.type,event.value);
            break;
        case CREATE_WORKSPACE:
            printf("Event Triggered %d | Created workspace: %d\n", event.type,event.value);
            break;
        case DESTROY_WORKSPACE:
            printf("Event Triggered %d | Destroyed workspace: %d\n", event.type,event.value);
            break;
        case INFO_WORKSPACE:
            printf("Event Triggered %d | Active Workspace: %d | Available Workspace: %d\n",event.type, *(workspace_data), *(workspace_data + 1));
            break;
    }

    cairo_set_source_rgba(appState->cai_context, 1.0, 105.0/255.0, 196.0/255.0, ALPHA);
    cairo_rectangle(appState->cai_context,0,0,200,appState->height);
    cairo_fill(appState->cai_context);

    if(available_workspace > 5) available_workspace = 5;
    
    for(int i = 0;i < available_workspace;i++){
        
        if(workspaces[i] == active_workspace){
            cairo_set_source_rgba(appState->cai_context, 1.0, 155.0/255.0, 166.0/255.0, ALPHA);
            cairo_rectangle(appState->cai_context,i*40,0,40,appState->height);
            cairo_fill(appState->cai_context);
            continue;
        }
        cairo_set_source_rgba(appState->cai_context, 1.0, 155.0/255.0, 166.0/255.0, ALPHA);
        cairo_rectangle(appState->cai_context,i*40,0,40,appState->height);
        cairo_set_line_width(appState->cai_context, 8.0);
        cairo_stroke(appState->cai_context);

        
    }

    wl_surface_attach(appState->surface,appState->buffer,0,0);
    wl_surface_damage(appState->surface, 0,  0, 200, appState->height);
    wl_surface_commit(appState->surface);

    return NULL;
}

void * handle_time(void * data){
    Event event = *(Event *) data;
    int minutes = event.value;
    struct AppState * appstate = event.appState;
    char clock_now[10];

    snprintf(clock_now, 9, "%0.2d:%0.2d", minutes/60,minutes%60);
    printf("Event Triggered %d | Time -> %0.2d:%0.2d\n", event.type,event.value/60,event.value%60);

    cairo_set_source_rgba(appstate->cai_context, 1, 105.0/255.0, 196.0/255.0, ALPHA);
    cairo_rectangle(appstate->cai_context,  920, 0, 200, appstate->height);
    cairo_fill(appstate->cai_context);

    cairo_set_source_rgba(appstate->cai_context, 1, 1, 1, ALPHA);
    cairo_move_to(appstate->cai_context, 920, appstate->height - 5);
    cairo_show_text(appstate->cai_context, clock_now);

    wl_surface_attach(appstate->surface,appstate->buffer,0,0);
    wl_surface_damage(appstate->surface, 920,  0, 200, appstate->height);
    wl_surface_commit(appstate->surface);


    return NULL;
}

void * handle_brightness(void * data){
    Event event = *(Event *) data;
    printf("Event Triggered %d | Brightness status: %d\n", event.type,event.value);

    return NULL;
}

void * handle_volume(void * data){
    Event event = *(Event *) data;
    printf("Event Triggered %d | Volume status: %d\n", event.type,event.value);
    
    return NULL;
}

void * handle_bluetooth(void * data){
    Event event = *(Event *) data;
    printf("Event Triggered %d | Bluetooth %s\n", event.type,event.value? "Connected":"Disconnected");

    return NULL;
}

void * handle_network(void * data){
    Event event = *(Event *) data;
    printf("Event Triggered %d | Wifi is %s\n", event.type,event.value? "Up":"Down");

    return NULL;
}

void * handle_power(void * data){
    Event event = *(Event *) data;
    switch (event.specifier) {
        case BATTERY_STATUS:
            printf("Event Triggered %d | Power Status: %d\n", event.type,event.value); 
            break;
        case CHARGE_STATUS:
            printf("Event Triggered %d | Charge Status: %d\n", event.type,event.value);  
            break;
    }
    return NULL;
}

void * handle_mpd(void * data){
    Event event = *(Event *)data;
    struct AppState * appstate = event.appState;

    switch (event.specifier) {
        case MPD_UP:
            printf("Event Triggered %d | Mpd is Up\n",event.type);
            break;
        case MPD_DOWN:
            printf("Event Triggered %d | Mpd is Down\n",event.type);
            break;
        case MPD_SENT:
            printf("Event Triggered %d | Mpd Player | Playing %s\n",event.type,event.data? event.data: "NULL");
            break;
        case MPD_ERR:
            printf("Event Triggered %d | Mpd Err\n", event.type);
            break;
    }

    if(event.specifier != MPD_SENT) return NULL;
    cairo_set_source_rgba(appstate->cai_context, 1, 105.0/255.0, 196.0/255.0, ALPHA);
    cairo_rectangle(appstate->cai_context,  250, 0, 600, appstate->height);
    cairo_fill(appstate->cai_context);

    cairo_set_font_size(appstate->cai_context,appstate->height-10);
    cairo_set_source_rgba(appstate->cai_context, 1, 1, 1, ALPHA);
    cairo_move_to(appstate->cai_context, 250, appstate->height - 10);
    cairo_show_text(appstate->cai_context, event.data);
    cairo_set_font_size(appstate->cai_context,appstate->height);

    wl_surface_attach(appstate->surface,appstate->buffer,0,0);
    wl_surface_damage(appstate->surface, 250,  0, 600, appstate->height);
    wl_surface_commit(appstate->surface);

    return NULL;
}