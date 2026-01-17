#define _GNU_SOURCE
#include <stdio.h>
#include <dlfcn.h>
#include "module_interface.h"


int main(void)
{
    void *handle;
    struct module_interface *(*mod_init)(
        int, const struct wb_public_api *
    );

    struct wb_public_api api = {0};
    struct module_interface *iface;

    handle = dlopen("libpower.so", RTLD_NOW);
    if (!handle) {
        fprintf(stderr, "dlopen failed: %s\n", dlerror());
        return 1;
    }

    dlerror();

    mod_init = dlsym(handle, "mod_init");
    if (!mod_init) {
        fprintf(stderr, "dlsym failed: %s\n", dlerror());
        return 1;
    }

    iface = mod_init(0, &api);
    printf("Module loaded: %s\n", iface->module_name);

    dlclose(handle);
    return 0;
}

