#include "../module_interface.h"
#include <stdlib.h>

struct module_interface *
mod_init(int id, const struct wb_public_api *api)
{
    (void)id;
    (void)api;

    struct module_interface *iface = malloc(sizeof(*iface));
    iface->module_name = "power";

    return iface;
}

