#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

#include "uthash.h"
#include "module.h"
#include "config.h"
#include "style.h"

extern const struct wb_config_api config_api;
extern const struct wb_mod_api mod_api;
extern const struct wb_widget_api widget_api;

const struct wb_public_api api = {
		.mod = &mod_api,
		.widget = &widget_api,
		.config = &config_api
};

struct module_interface **
load_modules(struct wb_config_setting * mods, int * count,
			struct wb_config_setting ** mod_sets, struct wb_appstate * appstate)
{
	struct module_interface ** interfaces;
	int mod_count = *count;
	int mod_found = 0;
	printf("mod_count = %d\n", mod_count);

	interfaces = malloc(sizeof(struct module_interface *) * mod_count);
	struct wb_style_sec * entry = NULL;

	const char * prefix = "lib";
	char path[128] = {0};
	void * handle = NULL;

    
	for (int i = 0; i < mod_count; i++){
		struct wb_config_setting * mod_fnd = wb_config_s_by_index(mods, i);

		struct module_interface * mod_int = NULL;
		struct module_interface * (* init)(int, const struct wb_public_api *) = NULL;
		const char * mod_name = wb_config_s_name(mod_fnd);

		snprintf(path, sizeof(path), "%s%s.so", prefix, mod_name);
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

		mod_int = init(mod_found, &api);
		interfaces[mod_found] = mod_int;
		printf("Module %s is Loaded | Id = %d\n", mod_int->module_name, mod_found);

		mod_int->id = mod_found;
		mod_int->api = &api;
		mod_sets[mod_found] = mod_fnd;
	}

	*count = mod_found;
	return interfaces;
}

