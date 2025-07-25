#include "../include/fetcher.h"
#include "../include/style.h"

static void parse_tu(struct component_style * sty, char * val){

    char * endptr = NULL;
   
    if(strchr(val,'.')){

        sty->db_val = strtod(val, &endptr);
        if (val == endptr) 
            goto is_str;
        
        sty->type = DOUBLE;

        if (*endptr == '%')
            sty->unit = PCTG;
        else if (strncmp(val,"px",2) == 0)
            sty->unit = PX;
        else 
            sty->unit = NONE;
    }

    else {
        sty->int_val = strtol(val,&endptr,10);
        if (val == endptr)
            goto is_str;

        sty->type = INT;

        if (*endptr == '%')
            sty->unit = PCTG;
        else if (strncmp(endptr,"px",2) == 0)
            sty->unit = PX;
        else
            sty->unit = NONE;
    }

    return;

    is_str:
    sty->unit = NONE;
    sty->type = STRING;

    strncpy(sty->str_val, val , 64);

    return;
}

static char * read_section(char * line, struct component_entries ** entries){

    struct component_entries * sec_ent = malloc(sizeof(struct component_entries));

    sec_ent->style = NULL;
    char * iter = line + 1;
    int end = strcspn(iter, "]");
    iter[end] = 0;
    
    // 32 byte is the size of the key propery
    snprintf(sec_ent->section_key, 32 ,"%s",iter); 

    HASH_ADD_STR(*entries, section_key, sec_ent); // take entries adrress as head, put sec_ent and section_key field intro entries
    return sec_ent->section_key;
}

static void read_entries(char * line,struct component_entries * entry ){

    char * key = line, * value;
    int delimit = strcspn(line, "=");

    struct component_style * dummy = NULL;
    struct component_style * style = malloc(sizeof(struct component_style));

    value = line + delimit + 1;
    value[strcspn(value, "\n")] = 0;
    key[delimit] = 0;

    // printf("Key: %s || Val:%s\n",key,value);

    HASH_FIND_STR(entry->style, key, dummy); 
    // take entry address as head, search key, and return it into dummy
    if(dummy){
        HASH_DEL(entry->style, dummy);
        free(dummy);
    }

    snprintf(style->key, 32, "%s", key);
    parse_tu(style, value);

    HASH_ADD_STR(entry->style, key, style);
    return;
}

static FILE * open_config(char * path){
    char full_path[128];
    char * home_path = getenv("HOME");

    snprintf(full_path, sizeof(full_path), "%s/%s", home_path,".config/waybro/pconfig");
    FILE * config_fd = fopen(full_path, "r");

    if (!config_fd) 
        ON_ERR("Error on Opening config file")

    return config_fd;
}

struct component_entries * read_config(char *path, struct AppState * appstate){

    struct component_entries * ent_head = NULL, * dummy;
    char buffer[512] = {0};
    char * section_now = NULL; 
    FILE * config_fd = open_config(path);

    while (fgets(buffer,sizeof(buffer),config_fd)) {

        buffer[strcspn(buffer, "\n")] = 0;
        
        if(strchr(buffer, '[')){

            dummy = NULL;
            section_now = read_section(buffer, &ent_head);
            HASH_FIND_STR(ent_head, section_now, dummy);
            
        }
        else if(strchr(buffer, '=')){
            if (!section_now)
                ON_ERR("Stray entries")
            read_entries(buffer,dummy);
        }

        memset(buffer, 0, sizeof(buffer));
    }

    fclose(config_fd);

    return ent_head;
}

static void match_sty(struct component_style * style,
                      char * str, struct m_style * mstyle){

    int val = style->int_val;
    
    if (strcmp(str, "width") == 0){
        mstyle->width = val;
    }
    else if (strcmp(str, "height") == 0){
        mstyle->height = val;
    }
    else if (strcmp(str, "a") == 0){
        mstyle->a = val;
    }
    else if (strcmp(str, "r") == 0){
        mstyle->r = val;
    }
    else if (strcmp(str, "g") == 0) {
        mstyle->g = val;
    }
    else if (strcmp(str, "b") == 0) {
        mstyle->b = val;
    }
    
}

struct m_style * translate_mstyle(struct component_entries ** entries){

    char key[][10] = {"width","height","a","r","g","b"};
    struct m_style * m_style = malloc(sizeof(struct m_style));
    struct m_style def_sty = {1920,30,80,105,105,196};

    //default style
    memcpy(m_style, &def_sty, sizeof(struct m_style));

    struct component_entries * ent_fnd = NULL;

    HASH_FIND_STR(*entries, "general", ent_fnd);
    if(!ent_fnd){
        return m_style;
    }

    struct component_style * sty_fnd = NULL;
    for(int i = 0; i < M_STYLE_COUNT; i++){
        HASH_FIND_STR(ent_fnd->style, key[i], sty_fnd);
        if(sty_fnd){
            match_sty(sty_fnd, key[i], m_style);
            HASH_DEL( ent_fnd->style, sty_fnd);
            free(sty_fnd);
            sty_fnd = NULL;
        }
    }

    HASH_DEL(*entries, ent_fnd);
    free(ent_fnd);

    return m_style;

}

