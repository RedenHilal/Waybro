#include "../include/style.h"
#include <stdio.h>

#define CLEAN_HASH(head,delptr) {HASH_DEL(head,delptr);free(delptr);}

static char * const base_match[] = {
    "width",
    "height",
    "x",
    "y",
    "font_size",
    "enabled",
    "rd_left",
    "rd_right"
};




static int calc_pcpx(enum style_unit unit, int raw_val, int main_val){
    if (unit == PCTG){
        return (main_val * raw_val) / 100;
    }
    else if (unit == PX){
        return raw_val;
    }
    else if (unit == NONE) {
        return raw_val;
    }

    return raw_val;
}

static void get_base_width(struct base_style * base,struct component_style * sty_fnd, 
                           struct m_style * main_sty){

    base->width = calc_pcpx(sty_fnd->unit, sty_fnd->int_val, main_sty->width);
}

static void get_base_height(struct base_style * base,struct component_style * sty_fnd, 
                           struct m_style * main_sty){

    base->height = calc_pcpx(sty_fnd->unit, sty_fnd->int_val, main_sty->height);
}

static void get_base_fs(struct base_style * base,struct component_style * sty_fnd, 
                           struct m_style * main_sty){

    base->font_size = calc_pcpx(sty_fnd->unit, sty_fnd->int_val, main_sty->height);
}

static void get_base_x(struct base_style * base,struct component_style * sty_fnd, 
                           struct m_style * main_sty){

    base->x = sty_fnd->int_val;
}

static void get_base_y(struct base_style * base,struct component_style * sty_fnd, 
                           struct m_style * main_sty){

    base->y = sty_fnd->int_val;
}

static void get_base_enabled(struct base_style * base,struct component_style * sty_fnd, 
                           struct m_style * main_sty){

    base->enabled = sty_fnd->int_val;
}

static void get_base_rdl(struct base_style * base,struct component_style * sty_fnd, 
                           struct m_style * main_sty){

    base->rd_left = sty_fnd->int_val;
}

static void get_base_rdr(struct base_style * base,struct component_style * sty_fnd, 
                           struct m_style * main_sty){

    printf("rdr_val: %d\n",sty_fnd->int_val);
    base->rd_right = sty_fnd->int_val;
}

static void(*base_dispatch[])(struct base_style *, struct component_style*, struct m_style*) = {
    get_base_width,
    get_base_height,
    get_base_x,
    get_base_y,
    get_base_fs,
    get_base_enabled,
    get_base_rdl,
    get_base_rdr
};

static inline void match_base_sty(struct base_style * base,struct component_style * sty_fnd,
                           int index, struct m_style * main_sty){

    base_dispatch[index](base,sty_fnd,main_sty);
}

static int get_fmt(char * dest, struct component_entries * entry){

    struct component_style * sty_fnd = NULL;
    HASH_FIND_STR(entry->style, "format", sty_fnd);

    if(!sty_fnd)
        return -1;

    
    strncpy(dest, sty_fnd->str_val, 64);
    return 0;
}

static void iter_ws_key(struct component_style * sty_fnd, int index,
                        struct ws_style * ws_sty){
    
    switch (index) {
        case 0:
            ws_sty->max_ws = sty_fnd->int_val;
            break;
        case 1:
            ws_sty->radius = sty_fnd->int_val;
            break;
        case 2:
            ws_sty->h_color = sty_fnd->int_val;
    }

}

static void get_ws_ent(struct component_entries * entry, struct ws_style * ws_sty){

    char ws_key[][10] = {
        "ws_max",
        "radius",
        "h_color"
    };

    struct component_style * sty_fnd = NULL;
    int size = sizeof(ws_key) / sizeof(ws_key[0]);

    for (int i = 0; i < size; i++){
        HASH_FIND_STR(entry->style, ws_key[i],sty_fnd);

        if(sty_fnd){
            iter_ws_key(sty_fnd, i, ws_sty);
            CLEAN_HASH(entry->style, sty_fnd);
        }

        sty_fnd = NULL;
    }

}

void get_base_sty(struct base_style * base, struct component_entries * entry,
                  struct m_style * main_sty){
    
    struct component_style * sty_fnd = NULL;
    int iter = sizeof(base_match)/sizeof(base_match[0]);

    for(int i = 0; i < iter; i++){

        HASH_FIND_STR(entry->style, base_match[i], sty_fnd);

        if(!sty_fnd){
            base->enabled = 0;
            continue;
        }

        match_base_sty(base, sty_fnd, i, main_sty);

        CLEAN_HASH(entry->style, sty_fnd);
        sty_fnd = NULL;
    }

}


void * get_ws_sty(struct component_entries ** entries, struct m_style * main_sty){

    char section[] = "workspace";
    struct ws_style * ws_sty = calloc(1,sizeof(struct ws_style));

    struct component_entries * sect_fnd = NULL;
    HASH_FIND_STR(*entries, section, sect_fnd);

    if(!sect_fnd){
        ws_sty->base.enabled = 0;
        return ws_sty;
    }

    get_base_sty(&ws_sty->base, sect_fnd, main_sty);
    printf("-=%d\n",ws_sty->base.rd_right);

    get_ws_ent(sect_fnd, ws_sty);
    CLEAN_HASH(*entries, sect_fnd);

    printf("max_ws:%d | radius: %d | color:%d\n",ws_sty->max_ws,ws_sty->radius,ws_sty->h_color);


    return ws_sty;
}

void * get_tm_sty(struct component_entries ** entries, struct m_style * main_sty){

    char section[] = "time";
    struct tm_style * tm_sty = calloc(1,sizeof(struct tm_style));

    struct component_entries * sect_fnd = NULL;
    HASH_FIND_STR(*entries, section, sect_fnd);

    if(!sect_fnd){
        tm_sty->base.enabled = 0;
        return tm_sty;
    }

    get_base_sty(&tm_sty->base, sect_fnd, main_sty);
    int res = get_fmt(tm_sty->format, sect_fnd);

    if(res < 0)
        tm_sty->base.enabled = 0;

    CLEAN_HASH(*entries, sect_fnd);

    return tm_sty;
}

void * get_brght_sty(struct component_entries ** entries, struct m_style * main_sty){

    char section[] = "brightness";
    struct brightness_style * brght_sty = calloc(1,sizeof(struct brightness_style));

    struct component_entries * sect_fnd = NULL;
    HASH_FIND_STR(*entries, section, sect_fnd);

    if(!sect_fnd){
        brght_sty->base.enabled = 0;
        return brght_sty;
    }

    get_base_sty(&brght_sty->base, sect_fnd, main_sty);
    int res = get_fmt(brght_sty->format, sect_fnd);

    if (res < 0)
        brght_sty->base.enabled = 0;

    CLEAN_HASH(*entries, sect_fnd);

    return brght_sty;
}

void * get_vol_sty(struct component_entries ** entries, struct m_style * main_sty){

    char section[] = "volume";
    struct vol_style * vol_sty = calloc(1,sizeof(struct vol_style));

    struct component_entries * sect_fnd = NULL;
    HASH_FIND_STR(*entries, section, sect_fnd);

    if(!sect_fnd){
        vol_sty->base.enabled = 0;
        return vol_sty;
    }

    get_base_sty(&vol_sty->base, sect_fnd, main_sty);
    int res = get_fmt(vol_sty->format, sect_fnd);

    if (res < 0)
        vol_sty->base.enabled = 0;

    CLEAN_HASH(*entries, sect_fnd);

    return vol_sty;
}

void * get_blue_sty(struct component_entries ** entries, struct m_style * main_sty){

    char section[] = "bluetooth";
    struct blue_style * blue_sty = calloc(1,sizeof(struct blue_style));

    struct component_entries * sect_fnd = NULL;
    HASH_FIND_STR(*entries, section, sect_fnd);

    if(!sect_fnd){
        blue_sty->base.enabled = 0;
        return blue_sty;
    }

    get_base_sty(&blue_sty->base, sect_fnd, main_sty);
    int res = get_fmt(blue_sty->format, sect_fnd);

    if (res < 0)
        blue_sty->base.enabled = 0;

    CLEAN_HASH(*entries, sect_fnd);

    return blue_sty;
}

void * get_net_sty(struct component_entries ** entries, struct m_style * main_sty){

    char section[] = "network";
    struct net_style * net_sty = calloc(1,sizeof(struct net_style));

    struct component_entries * sect_fnd = NULL;
    HASH_FIND_STR(*entries, section, sect_fnd);

    if(!sect_fnd){
        net_sty->base.enabled = 0;
        return net_sty;
    }

    get_base_sty(&net_sty->base, sect_fnd, main_sty);
    int res = get_fmt(net_sty->format, sect_fnd);

    if (res < 0)
        net_sty->base.enabled = 0;

    CLEAN_HASH(*entries, sect_fnd);

    return net_sty;
}

void * get_power_sty(struct component_entries ** entries, struct m_style * main_sty){

    char section[] = "power";
    struct power_style * pow_sty = calloc(1,sizeof(struct power_style));

    struct component_entries * sect_fnd = NULL;
    HASH_FIND_STR(*entries, section, sect_fnd);

    if(!sect_fnd){
        pow_sty->base.enabled = 0;
        return pow_sty;
    }

    get_base_sty(&pow_sty->base, sect_fnd, main_sty);
    int res = get_fmt(pow_sty->format, sect_fnd);

    if (res < 0)
        pow_sty->base.enabled = 0;

    CLEAN_HASH(*entries, sect_fnd);

    return pow_sty;
}

void * get_mpd_sty(struct component_entries ** entries, struct m_style * main_sty){

    char section[] = "mpd";
    struct mpd_style * mpd_sty = calloc(1,sizeof(struct mpd_style));

    struct component_entries * sect_fnd = NULL;
    HASH_FIND_STR(*entries, section, sect_fnd);

    if(!sect_fnd){
        mpd_sty->base.enabled = 0;
        return mpd_sty;
    }

    get_base_sty(&mpd_sty->base, sect_fnd, main_sty);
    int res = get_fmt(mpd_sty->format, sect_fnd);

    if (res < 0)
        mpd_sty->base.enabled = 0;

    CLEAN_HASH(*entries, sect_fnd);

    return mpd_sty;
}

void * get_sys_sty(struct component_entries ** entries, struct m_style * main_sty){
    return NULL;
}