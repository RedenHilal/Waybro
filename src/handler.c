#include "../include/fetcher.h"

cairo_surface_t * resources;

static void erase_rect_area(struct AppState* appstate, int x,int y,int width, int height){
    cairo_save(appstate->cai_context);
    cairo_set_operator(appstate->cai_context, CAIRO_OPERATOR_CLEAR);
    cairo_rectangle(appstate->cai_context, x, y, width, height);
    cairo_fill(appstate->cai_context);
    cairo_restore(appstate->cai_context);
}

static void expose_rect_area(struct AppState * appstate,int x,int y, int width, int height){
    wl_surface_attach(appstate->surface,appstate->buffer,0,0);
    wl_surface_damage(appstate->surface, x,  y, width, height);
    wl_surface_commit(appstate->surface);
}

static void render_text(struct AppState * appstate, int x, int y, int fontsize, const char* string){
    //cairo_select_font_face(appstate->cai_context, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(appstate->cai_context,fontsize);
    cairo_set_source_rgba(appstate->cai_context, 1, 1, 1, ALPHA);
    cairo_move_to(appstate->cai_context, x, y);
    cairo_show_text(appstate->cai_context, string);
}

static void draw_rect(struct AppState * appstate, int x, int y, int width, int height){
    cairo_set_source_rgba(appstate->cai_context, 1.0, 105.0/255.0, 196.0/255.0, ALPHA);
    cairo_rectangle(appstate->cai_context, x, y, width, height);
    cairo_fill(appstate->cai_context);
}

static void render_resource(void * data, const char * path, int x, int y, int width,int height){
    struct AppState * appstate = data;
    resources = cairo_image_surface_create_from_png(path);
    if(cairo_surface_status(resources) != CAIRO_STATUS_SUCCESS)
        ON_ERR("load resources - handler")

    cairo_save(appstate->cai_context);
    int img_width = cairo_image_surface_get_width(resources);
    int img_height = cairo_image_surface_get_height(resources);

    cairo_translate(appstate->cai_context, x, y);
    cairo_scale(appstate->cai_context, (double)width/img_width, (double)width/img_height);
    cairo_set_source_rgba(appstate->cai_context, 1, 1, 1, ALPHA);
    cairo_mask_surface(appstate->cai_context, resources, 0, 0);

    cairo_restore(appstate->cai_context);
}

void resources_init(void * data){
    render_resource(data, "../build/resources/battery-mid-svgrepo-com.png", 1820, 0, 30, 30);
    render_resource(data, "../build/resources/bluetooth-on-svgrepo-com.png", 1760,3, 24, 24);
    render_resource(data, "../build/resources/wifi-1018-svgrepo-com.png", 1700, 3, 24, 24);
    render_resource(data, "../build/resources/volume-high-svgrepo-com.png", 1600, 3, 24, 24);
    render_resource(data, "../build/resources/brightness-svgrepo-com.png", 1540, 3, 24, 24);
}

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

    // Read workspaces info from fetcher
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

    erase_rect_area(appState, 0, 0, 200, appState->height);

    draw_rect(appState, 0, 0, 200, appState->height);

    if(available_workspace > 6) available_workspace = 6;
    
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

    expose_rect_area(appState, 0, 0, 200, appState->height);

    return NULL;
}

void * handle_time(void * data){
    Event event = *(Event *) data;
    int minutes = event.value;
    struct AppState * appstate = event.appState;
    char clock_now[10];

    snprintf(clock_now, 9, "%0.2d:%0.2d", minutes/60,minutes%60);
    printf("Event Triggered %d | Time -> %0.2d:%0.2d\n", event.type,event.value/60,event.value%60);

    erase_rect_area(appstate, 920, 0, 200, appstate->height);

    draw_rect(appstate, 920, 0, 200, appstate->height);

    render_text(appstate, 920, appstate->height-5, appstate->height, clock_now);

    expose_rect_area(appstate, 920, 0, 200, appstate->height);

    return NULL;
}

void * handle_brightness(void * data){
    Event event = *(Event *) data;
    struct AppState * appstate = event.appState;
    char buffer[16];

    printf("Event Triggered %d | Brightness status: %d\n", event.type,event.value);

    snprintf(buffer, sizeof(buffer) - 1, "%d%%", event.value);

    erase_rect_area(appstate, 1570, 0, 30, appstate->height);
    draw_rect(appstate, 1570, 0, 30, appstate->height);
    render_text(appstate, 1570, appstate->height - 10, appstate->height - 15, buffer);
    expose_rect_area(appstate, 1540, 0, 60, appstate->height);

    return NULL;
}

void * handle_volume(void * data){
    Event event = *(Event *) data;
    struct AppState * appstate = event.appState;
    char buffer[16];

    printf("Event Triggered %d | Volume status: %d\n", event.type,event.value);
    snprintf(buffer, sizeof(buffer) - 1, "%d%%", event.value);

    erase_rect_area(appstate, 1630, 0, 30, appstate->height);
    draw_rect(appstate, 1630, 0, 30, appstate->height);
    render_text(appstate, 1630, appstate->height - 10, appstate->height - 15, buffer);
    expose_rect_area(appstate, 1600, 0, 60, appstate->height);
    
    return NULL;
}

void * handle_bluetooth(void * data){
    Event event = *(Event *) data;
    struct AppState * appstate = event.appState;
    int * connections = (int *)event.data;
    char buffer[16];

    snprintf(buffer, sizeof(buffer) - 1, "%d", *connections);
    printf("Event Triggered %d | Bluetooth Connection: %d\n", event.type,*connections);

    erase_rect_area(appstate, 1790, 0, 30, appstate->height);
    draw_rect(appstate, 1790, 0, 30, appstate->height);
    render_text(appstate, 1790, appstate->height - 10, appstate->height-15, buffer);
    expose_rect_area(appstate, 1760, 0, 60, appstate->height);

    return NULL;
}

void * handle_network(void * data){
    Event event = *(Event *) data;
    struct AppState * appstate = event.appState;
    
    printf("Event Triggered %d | Wifi is %s\n", event.type,event.value? "Up":"Down");

    erase_rect_area(appstate, 1730, 0, 30, appstate->height);
    draw_rect(appstate, 1730, 0, 30, appstate->height);
    render_text(appstate, 1730, appstate->height - 10, appstate->height - 15, event.value?"Up":"Down");
    expose_rect_area(appstate, 1700, 0, 60, appstate->height);

    return NULL;
}

void * handle_power(void * data){
    Event event = *(Event *) data;
    struct AppState * appstate = event.appState;
    char power[6];

    switch (event.specifier) {
        case BATTERY_STATUS:
            printf("Event Triggered %d | Power Status: %d\n", event.type,event.value); 
            break;
        case CHARGE_STATUS:
            printf("Event Triggered %d | Charge Status: %d\n", event.type,event.value);  
            break;
    }

    if(event.specifier == CHARGE_STATUS) return NULL;

    snprintf(power, 5, "%d%%", event.value);

    erase_rect_area(appstate, 1850, 0, 90, appstate->height);

    draw_rect(appstate, 1850, 0, 90, appstate->height);

    render_text(appstate, 1850, appstate->height - 10, appstate->height-15, power);

    expose_rect_area(appstate, 1820, 0, 70, appstate->height);

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

    erase_rect_area(appstate, 250, 0, 600, appstate->height);

    cairo_set_source_rgba(appstate->cai_context, 1, 105.0/255.0, 196.0/255.0, ALPHA);
    cairo_rectangle(appstate->cai_context,  250, 0, 600, appstate->height);
    cairo_fill(appstate->cai_context);

    PangoLayout * layout = pango_cairo_create_layout(appstate->cai_context);

	pango_layout_set_text(layout, event.data, -1);
	pango_layout_set_font_description(layout, pango_font_description_from_string("Sans 17"));

	cairo_set_source_rgba(appstate->cai_context, 1, 1, 1, ALPHA);
	cairo_move_to(appstate->cai_context, 250, 0);
    
	pango_cairo_show_layout(appstate->cai_context, layout);

    expose_rect_area(appstate, 250, 0, 600, appstate->height);

	g_object_unref(layout);

    return NULL;
}
