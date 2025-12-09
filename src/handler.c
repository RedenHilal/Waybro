#include "fetcher.h"
#include "style.h"
#include "core.h"

cairo_surface_t * resources;

static void draw_arc(cairo_t * cai_context, struct base_style * base, struct m_style * m_sty, int width){
    
    int lx = base->x  + base->rd_left;
    int rx = width + base->rd_left + base->x;
    int y = base->y + base->height/2;
   
    cairo_set_source_rgba(cai_context, TO_RGB_FMT(m_sty->r), TO_RGB_FMT(m_sty->g),
                        TO_RGB_FMT(m_sty->b), TO_ALPHA(m_sty->a));  
                    
    cairo_new_path(cai_context);
    cairo_move_to(cai_context, lx, base->y + base->height);

    if (base->rd_left)
        cairo_arc(cai_context, lx, y, base->rd_left, M_PI_2, 3 * M_PI_2);
    else
        cairo_line_to(cai_context, lx, base->y);

    cairo_line_to(cai_context, rx, base->y);

    if (base->rd_right)
        cairo_arc(cai_context, rx, y, base->rd_right, -M_PI_2, M_PI_2);
    else
        cairo_line_to(cai_context, rx, base->y + base->height);

    cairo_line_to(cai_context, lx,base->y + base->height);

    cairo_close_path(cai_context);
    cairo_fill(cai_context);
    
}


static void get_center(PangoLayout * layout, int * x, int * y, int width, int height, int dx, int dy){
    
    int lwidth, lheight;
    pango_layout_get_size(layout, &lwidth,&lheight);

    lwidth /= PANGO_SCALE;
    lheight /= PANGO_SCALE;

    *x = dx + (width - lwidth) / 2;
    *y = dy + (height - lheight) / 2;
}

static void draw_text(cairo_t * cai_context, struct base_style * style, char * text){

    int x, y;
    
    PangoLayout * layout = pango_cairo_create_layout(cai_context);
    
	pango_layout_set_text(layout, text, -1);
	pango_layout_set_font_description(layout, pango_font_description_from_string("Comic Neue 11"));

    get_center( layout, &x, &y, style->width,style->height,style->x,style->y);

	cairo_set_source_rgba(cai_context, 1, 1, 1, ALPHA);
	cairo_move_to(cai_context, x, y);
    
	pango_cairo_show_layout(cai_context, layout);

    g_object_unref(layout);    
}

static void draw_text_xy(cairo_t * cai_context, char * text,int x,int y, int width, int height){

    int cx,cy;

    PangoLayout * layout = pango_cairo_create_layout(cai_context);
    
	pango_layout_set_text(layout, text, -1);
	pango_layout_set_font_description(layout, pango_font_description_from_string("Comic Neue 11"));

    get_center(layout, &cx, &cy, width, height, x, y);

	cairo_set_source_rgba(cai_context, 1, 1, 1, ALPHA);
	cairo_move_to(cai_context, cx, cy);
    
	pango_cairo_show_layout(cai_context, layout);

    g_object_unref(layout);    
}

static int get_prop_int(struct component_entries * entries,char * entry_name, char * field_name){

    struct component_entries * ent_fnd = NULL;
    HASH_FIND_STR(entries, entry_name, ent_fnd);

    if (!ent_fnd){
        return -1;
    }

    struct component_style * sty_fnd = NULL;
    HASH_FIND_STR(ent_fnd->style, field_name, sty_fnd);

    if (!sty_fnd){
        return -1;
    }

}

static char * get_str_by_format(char * format, char * str_val){

    char * begin = strchr(format, '{');
    char * end = strchr(begin, '}');

    int pre_len = begin - format;
    int post_len = strlen(end + 1);
    int val_len = strlen(str_val);

    int total_len = pre_len + post_len + val_len + 1;
    char * str_fmt = malloc(total_len);

    snprintf(str_fmt, total_len, "%.*s%s%s", pre_len, format, str_val, end);

    return str_fmt;
}

static inline void erase_rect_area(struct AppState* appstate, int x,int y,int width, int height){
    cairo_save(appstate->cai_context);
    cairo_set_operator(appstate->cai_context, CAIRO_OPERATOR_CLEAR);
    cairo_rectangle(appstate->cai_context, x, y, width, height);
    cairo_fill(appstate->cai_context);
    cairo_restore(appstate->cai_context);
}

		static inline void expose_rect_area(struct AppState * appstate,int x,int y, int width, int height){
			wl_surface_attach(appstate->surface,appstate->buffer,0,0);
			wl_surface_damage(appstate->surface, x,  y, width, height);
			wl_surface_commit(appstate->surface);
		}

		static inline void render_text(struct AppState * appstate, int x, int y, int fontsize, const char* string){
			//cairo_select_font_face(appstate->cai_context, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
			cairo_set_font_size(appstate->cai_context,fontsize);
			cairo_set_source_rgba(appstate->cai_context, 1, 1, 1, ALPHA);
			cairo_move_to(appstate->cai_context, x, y);
			cairo_show_text(appstate->cai_context, string);
		}

		static inline void draw_rect(struct AppState * appstate, int x, int y, int width, int height){
			
			struct m_style * m_sty = appstate->m_style;
			cairo_set_source_rgba(appstate->cai_context, TO_RGB_FMT(m_sty->r), TO_RGB_FMT(m_sty->g),
								  TO_RGB_FMT(m_sty->b), TO_ALPHA(m_sty->a));

			cairo_rectangle(appstate->cai_context, x, y, width, height);
			cairo_fill(appstate->cai_context);
		}

		static inline void draw_rect_spc(struct AppState * appstate, int x, int y, int width, int height,
								  uint8_t r, uint8_t g, uint8_t b, uint8_t a){
			cairo_set_source_rgba(appstate->cai_context, TO_RGB_FMT(r), TO_RGB_FMT(g),
								  TO_RGB_FMT(b), TO_ALPHA(a));

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
    render_resource(data, "../build/resources/wifi-1018-svgrepo-com.png", 1685, 3, 24, 24);
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
    struct AppState * appstate = event.appstate;
    struct ws_style * style = event.styles;
    struct base_style * base = &style->base;
    struct m_style * main_sty = appstate->m_style;

    int * workspace_data = (int*)event.data;
    char buffer[4];

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
            continue;
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

    int rect_x = base->x + base->rd_left;
    int total_width = base->rd_left + base->width + base->rd_right;
    int radius = style->radius;
    int diameter = radius * 2;

    erase_rect_area(appstate, base->x, 0, total_width, base->height);

    //draw_rect(appstate, rect_x, base->y, radius * 2 * available_workspace, base->height);
    draw_arc(appstate->cai_context, base, main_sty, radius * 2 * available_workspace);

    
    for(int i = 0;i < available_workspace;i++){

        snprintf(buffer, sizeof(buffer), "%d", workspaces[i]);
        draw_text_xy(appstate->cai_context, buffer, rect_x + i * diameter, base->y, diameter, base->height);
        
        if(workspaces[i] == active_workspace){
            cairo_set_source_rgba(appstate->cai_context, GET_RED(style->h_color), GET_GREEN(style->h_color),
            GET_BLUE(style->h_color), TO_ALPHA(main_sty->a));
            cairo_arc(appstate->cai_context, rect_x + i * diameter + radius, base->y + radius, radius, 0, 2 * M_PI);
            cairo_fill(appstate->cai_context);
            continue;
        }

        

    }

    expose_rect_area(appstate, base->x , 0, total_width , base->height);

    return NULL;
}

void * handle_time(void * data){

    Event event = *(Event *) data;


    int minutes = event.value;
    struct AppState * appstate = event.appstate;
    struct m_style * m_sty = appstate->m_style;
    struct tm_style * style = event.styles;
    struct base_style * base = &style->base;

    char clock_now[10];

    snprintf(clock_now, 9, "%0.2d:%0.2d", (minutes / 60) % 24, minutes % 60);
    printf("Event Triggered %d | Time -> %0.2d:%0.2d\n", event.type,event.value/60,event.value%60);

    int rect_x = base->x + base->rd_left;
    int total_width = base->width + base->rd_right + base->rd_left;
    char * text = get_str_by_format(style->format, clock_now);

    erase_rect_area(appstate, base->x , base->y, total_width, base->height);
    draw_arc(appstate->cai_context, base, m_sty, base->width);

    draw_text(appstate->cai_context, &style->base, text);
    free(text);

    expose_rect_area(appstate, base->x, base->y, total_width, base->height);

    return NULL;
}

void * handle_brightness(void * data){


    Event event = *(Event *) data;
    struct AppState * appstate = event.appstate;

    struct brightness_style * style = event.styles;
    struct base_style * base = &style->base;
    struct m_style * m_sty = appstate->m_style;
    char buffer[16];

    printf("Event Triggered %d | Brightness status: %d\n", event.type,event.value);

    snprintf(buffer, sizeof(buffer) - 1, "%d%%", event.value);

    int rect_x = base->x + base->rd_left;
    int total_width = base->width + base->rd_right + base->rd_left;
    char * text = get_str_by_format(style->format, buffer);

    erase_rect_area(appstate, base->x , base->y, total_width, base->height);
    draw_arc(appstate->cai_context, base, m_sty, base->width);

    draw_text(appstate->cai_context, &style->base, text);
    free(text);

    expose_rect_area(appstate, base->x, base->y, total_width, base->height);

    return NULL;
}

void * handle_volume(void * data){

    Event event = *(Event *) data;
    struct AppState * appstate = event.appstate;

    struct vol_style * style = event.styles;
    struct base_style * base = &style->base;
    struct m_style * m_sty = appstate->m_style;
    char buffer[16];

    printf("Event Triggered %d | Volume status: %d\n", event.type,event.value);
    snprintf(buffer, sizeof(buffer) - 1, "%d%%", event.value);

    int rect_x = base->x + base->rd_left;
    int total_width = base->width + base->rd_right + base->rd_left;
    char * text = get_str_by_format(style->format, buffer);

    erase_rect_area(appstate, base->x , base->y, total_width, base->height);
    draw_arc(appstate->cai_context, base, m_sty, base->width);

    draw_text(appstate->cai_context, &style->base, text);
    free(text);

    expose_rect_area(appstate, base->x, base->y, total_width, base->height);

    
    return NULL;
}

void * handle_bluetooth(void * data){

    Event event = *(Event *) data;
    struct AppState * appstate = event.appstate;

    struct m_style * m_sty = appstate->m_style;
    struct blue_style * style = event.styles;
    struct base_style * base = &style->base;

    int * connections = (int *)event.data;
    char buffer[16];

    snprintf(buffer, sizeof(buffer) - 1, "%d", *connections);
    printf("Event Triggered %d | Bluetooth Connection: %d\n", event.type,*connections);

    int rect_x = base->x + base->rd_left;
    int total_width = base->width + base->rd_right + base->rd_left;
    char * text = get_str_by_format(style->format, buffer);

    erase_rect_area(appstate, base->x , base->y, total_width, base->height);
    draw_arc(appstate->cai_context, base, m_sty, base->width);

    draw_text(appstate->cai_context, &style->base, text);
    free(text);

    expose_rect_area(appstate, base->x, base->y, total_width, base->height);

    return NULL;
}

void * handle_network(void * data){

    Event event = *(Event *) data;
    struct AppState * appstate = event.appstate;
    struct net_style * style = event.styles;
    struct base_style * base = &style->base;
    struct m_style * m_sty = appstate->m_style;

    printf("Event Triggered %d | Wifi is %s\n", event.type, event.value? "Up":"Down");
    char * status = event.value? "Up": "Down";
    
    int rect_x = base->x + base->rd_left;
    int total_width = base->width + base->rd_right + base->rd_left;
    char * text = get_str_by_format(style->format, status);

    erase_rect_area(appstate, base->x , base->y, total_width, base->height);
    draw_arc(appstate->cai_context, base, m_sty, base->width);

    draw_text(appstate->cai_context, &style->base, text);
    free(text);

    expose_rect_area(appstate, base->x, base->y, total_width, base->height);

    return NULL;
}

void * handle_power(void * data){

    
    Event event = *(Event *) data;
    struct AppState * appstate = event.appstate;
    struct m_style * m_sty = appstate->m_style;
    struct power_style * style = event.styles;
    struct base_style * base = &style->base;
    char power[6];

    switch (event.specifier) {
        case BATTERY_STATUS:
            printf("Event Triggered %d | Power Status: %d\n", event.type,event.value); 
            break;
        case CHARGE_STATUS:
            printf("Event Triggered %d | Charge Status: %d\n", event.type,event.value);  
            break;
    }
    

    if(event.specifier == CHARGE_STATUS) 
        return NULL;

    snprintf(power, 5, "%d%%", event.value);

    int rect_x = base->x + base->rd_left;
    int total_width = base->width + base->rd_right + base->rd_left;
    char * text = get_str_by_format(style->format, power);

    erase_rect_area(appstate, base->x , base->y, total_width, base->height);
    draw_arc(appstate->cai_context, base, m_sty, base->width);

    draw_text(appstate->cai_context, &style->base, text);
    free(text);

    expose_rect_area(appstate, base->x, base->y, total_width, base->height);


    return NULL;
}

void * handle_mpd(void * data){

    Event event = *(Event *)data;
    struct mpd_style * style = event.styles;
    struct base_style * base = &style->base;
    struct AppState * appstate = event.appstate;
    struct m_style * m_sty = appstate->m_style;

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

    if (event.specifier != MPD_SENT) 
        return NULL;

    int rect_x = base->x + base->rd_left;
    int total_width = base->width + base->rd_right + base->rd_left;
    char * text = get_str_by_format(style->format, event.data);


    erase_rect_area(appstate, base->x , base->y, total_width, base->height);
    draw_arc(appstate->cai_context, base, m_sty, base->width);

    draw_text(appstate->cai_context, &style->base, text);
    free(text);

    expose_rect_area(appstate, base->x, base->y, total_width, base->height);


    return NULL;
}

void * handle_mem(void * data){

    Event event = *(Event *)data;
    struct mem_style * style = event.styles;
    struct base_style * base = &style->base;
    struct AppState * appstate = event.appstate;
    struct m_style * m_sty = appstate->m_style;

    char buffer[16];
    
    int mem = event.value;

    printf("Event Triggered %d | Memory: %d%%\n",event.type,mem);

    snprintf(buffer, sizeof(buffer), "%d%%", mem);

    int rect_x = base->x + base->rd_left;
    int total_width = base->width + base->rd_right + base->rd_left;
    char * text = get_str_by_format(style->format, buffer);

    erase_rect_area(appstate, base->x , base->y, total_width, base->height);
    draw_arc(appstate->cai_context, base, m_sty, base->width);

    draw_text(appstate->cai_context, &style->base, text);
    free(text);

    expose_rect_area(appstate, base->x, base->y, total_width, base->height);


    return NULL;

}

void * handle_temp(void * data){

    Event event = *(Event *)data;
    struct temp_style * style = event.styles;
    struct base_style * base = &style->base;
    struct AppState * appstate = event.appstate;
    struct m_style * m_sty = appstate->m_style;

    char buffer[16];
    
    int temp = event.value;

    printf("Event Triggered %d | Temp: %d%%\n",event.type,temp);
    snprintf(buffer, sizeof(buffer), "%d%%", temp);

    int rect_x = base->x + base->rd_left;
    int total_width = base->width + base->rd_right + base->rd_left;
    char * text = get_str_by_format(style->format, buffer);

    erase_rect_area(appstate, base->x , base->y, total_width, base->height);
    draw_arc(appstate->cai_context, base, m_sty, base->width);

    draw_text(appstate->cai_context, &style->base, text);
    free(text);

    expose_rect_area(appstate, base->x, base->y, total_width, base->height);


    return NULL;

}



