#include "style.h"
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

static void get_base_width(struct wb_style_base * base,struct wb_style_unit * sty_fnd, 
                           struct wb_style_main * main_sty){

    base->width = calc_pcpx(sty_fnd->unit, sty_fnd->int_val, main_sty->width);
}

static void get_base_height(struct wb_style_base * base,struct wb_style_unit * sty_fnd, 
                           struct wb_style_main * main_sty){

    base->height = calc_pcpx(sty_fnd->unit, sty_fnd->int_val, main_sty->height);
}

static void get_base_fs(struct wb_style_base * base,struct wb_style_unit * sty_fnd, 
                           struct wb_style_main * main_sty){

    base->font_size = calc_pcpx(sty_fnd->unit, sty_fnd->int_val, main_sty->height);
}

static void get_base_x(struct wb_style_base * base,struct wb_style_unit * sty_fnd, 
                           struct wb_style_main * main_sty){

    base->x = sty_fnd->int_val;
}

static void get_base_y(struct wb_style_base * base,struct wb_style_unit * sty_fnd, 
                           struct wb_style_main * main_sty){

    base->y = sty_fnd->int_val;
}

static void get_base_enabled(struct wb_style_base * base,struct wb_style_unit * sty_fnd, 
                           struct wb_style_main * main_sty){

    base->enabled = sty_fnd->int_val;
}

static void get_base_rdl(struct wb_style_base * base,struct wb_style_unit * sty_fnd, 
                           struct wb_style_main * main_sty){

    base->rd_left = sty_fnd->int_val;
}

static void get_base_rdr(struct wb_style_base * base,struct wb_style_unit * sty_fnd, 
                           struct wb_style_main * main_sty){

    printf("rdr_val: %d\n",sty_fnd->int_val);
    base->rd_right = sty_fnd->int_val;
}

static void(*base_dispatch[])(struct wb_style_base *, struct wb_style_unit*, struct wb_style_main*) = {
    get_base_width,
    get_base_height,
    get_base_x,
    get_base_y,
    get_base_fs,
    get_base_enabled,
    get_base_rdl,
    get_base_rdr
};

static inline void match_base_sty(struct wb_style_base * base,struct wb_style_unit * sty_fnd,
                           int index, struct wb_style_main * main_sty){

    base_dispatch[index](base,sty_fnd,main_sty);
}

static int get_fmt(char * dest, struct wb_style_sec * entry){

    struct wb_style_unit * sty_fnd = NULL;
    HASH_FIND_STR(entry->style, "format", sty_fnd);

    if(!sty_fnd)
        return -1;

    
    strncpy(dest, sty_fnd->str_val, 64);
    return 0;
}

static int get_interval( struct wb_style_sec * entry){

    struct wb_style_unit * sty_fnd = NULL;
    HASH_FIND_STR(entry->style, "interval", sty_fnd);

    if (!sty_fnd)
        return 60;

    return sty_fnd->int_val;
}



void get_base_sty(struct wb_style_base * base, struct wb_style_sec * entry,
                  struct wb_style_main * main_sty){
    
    struct wb_style_unit * sty_fnd = NULL;
    int iter = sizeof(base_match)/sizeof(base_match[0]);

    for(int i = 0; i < iter; i++){

        HASH_FIND_STR(entry->style, base_match[i], sty_fnd);

        if(!sty_fnd){
            base->enabled = 0;
            continue;
        }

        match_base_sty(base, sty_fnd, i, main_sty);
        sty_fnd = NULL;
    }

}



void * get_tm_sty(struct wb_style_sec ** entries, struct wb_style_main * main_sty){

    char section[] = "time";
    struct tm_style * tm_sty = calloc(1,sizeof(struct tm_style));

    struct wb_style_sec * sect_fnd = NULL;
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

void * get_brght_sty(struct wb_style_sec ** entries, struct wb_style_main * main_sty){

    char section[] = "brightness";
    struct brightness_style * brght_sty = calloc(1,sizeof(struct brightness_style));

    struct wb_style_sec * sect_fnd = NULL;
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

void * get_vol_sty(struct wb_style_sec ** entries, struct wb_style_main * main_sty){

    char section[] = "volume";
    struct vol_style * vol_sty = calloc(1,sizeof(struct vol_style));

    struct wb_style_sec * sect_fnd = NULL;
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

void * get_blue_sty(struct wb_style_sec ** entries, struct wb_style_main * main_sty){

    char section[] = "bluetooth";
    struct blue_style * blue_sty = calloc(1,sizeof(struct blue_style));

    struct wb_style_sec * sect_fnd = NULL;
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

void * get_net_sty(struct wb_style_sec ** entries, struct wb_style_main * main_sty){

    char section[] = "network";
    struct net_style * net_sty = calloc(1,sizeof(struct net_style));

    struct wb_style_sec * sect_fnd = NULL;
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


void * get_mpd_sty(struct wb_style_sec ** entries, struct wb_style_main * main_sty){

    char section[] = "mpd";
    struct mpd_style * mpd_sty = calloc(1,sizeof(struct mpd_style));

    struct wb_style_sec * sect_fnd = NULL;
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

void * get_sys_sty(struct wb_style_sec ** entries, struct wb_style_main * main_sty){
    return NULL;
}

void * get_mem_sty(struct wb_style_sec ** entries, struct wb_style_main * main_sty){
    char section[] = "memory";
    struct mem_style * mem_sty = calloc(1, sizeof(struct mem_style));

    struct wb_style_sec * sect_fnd = NULL;
    HASH_FIND_STR(*entries, section, sect_fnd);

    if(!sect_fnd){
        mem_sty->base.enabled = 0;
        mem_sty->it_sec = 10;
        return mem_sty;
    }

    get_base_sty(&mem_sty->base, sect_fnd, main_sty);
    int res = get_fmt(mem_sty->format, sect_fnd);

    if (res < 0)
        mem_sty->base.enabled = 0;

    res = get_interval(sect_fnd);
    mem_sty->it_sec = res;

    CLEAN_HASH(*entries, sect_fnd);

    return mem_sty;

}

void * get_temp_sty(struct wb_style_sec ** entries, struct wb_style_main * main_sty){
    char section[] = "temp";
    struct temp_style * temp_sty = calloc(1, sizeof(struct temp_style));

    struct wb_style_sec * sect_fnd = NULL;
    HASH_FIND_STR(*entries, section, sect_fnd);

    if(!sect_fnd){
        temp_sty->base.enabled = 0;
        temp_sty->it_sec = 10;
        return temp_sty;
    }

    get_base_sty(&temp_sty->base, sect_fnd, main_sty);
    int res = get_fmt(temp_sty->format, sect_fnd);

    if (res < 0)
        temp_sty->base.enabled = 0;

    res = get_interval(sect_fnd);
    temp_sty->it_sec = res;

    CLEAN_HASH(*entries, sect_fnd);

    return temp_sty;
}
