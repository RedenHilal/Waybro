#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

#include "uthash.h"
#include "module.h"
#include "style.h"
#include "api-table.h"

const struct wb_public_api api = {
	.style = &wb_style_api_table,
	.render = &wb_render_api_table,
	.mod = &wb_mod_api_table
};

struct module_interface ** load_modules(struct wb_style_sec * cpn_entries,
										int * mod_found){
	int idx = 0;
	int count = HASH_COUNT(cpn_entries);
	struct module_interface ** interfaces;
	printf("mod_count = %d\n", count);

	interfaces = malloc(sizeof(struct module_interface *) * count);
	struct wb_style_sec * entry = NULL;

	const char * prefix = "lib";
	char path[128] = {0};
	void * handle = NULL;

    
	for (entry = cpn_entries; entry != NULL; entry = entry->hh.next){
		struct module_interface * mod_int = NULL;
		struct module_interface * (* init)(int, const struct wb_public_api *) = NULL;

		snprintf(path, sizeof(path), "%s%s.so", prefix, 
				 entry->section_key);
		handle = dlopen(path, RTLD_NOW);

		if (handle == NULL){
			printf("Failed to load Module %s\nErr: %s\n", path, dlerror());
			continue;
		}
			

		init = dlsym(handle, "mod_init");
		if (init == NULL){
			printf("Failed to init Module %s\nErr: %s\n", path, dlerror());
			continue;
		}
		mod_int = init(idx, &api);
		interfaces[idx] = mod_int;
		idx++;
		printf("Module %s is Loaded\n", mod_int->module_name);
	}
	*mod_found = idx;
	return interfaces;
}

//void iterate_styles(struct wb_style_sec * styles, int mod_count){
//	
//	for (int i = 0; i < mod_count; i++){
//		struct module_interface * mod_int = object[i].mod_int;
//		const char * mod_name = mod_int->module_name;
//
//		struct wb_style_sec * style;
//		HASH_FIND_STR(styles, mod_name, style);
//
//		mod_int->parse_sty(style);
//	}
//}
