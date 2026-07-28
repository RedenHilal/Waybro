#include <stdlib.h>
#include <unistd.h>
#include <systemd/sd-bus.h>

#include "utlist.h"
#include "macro.h"
#include "module.h"

void bluetooth_parse_sty(struct wb_config_setting * set, struct wb_style_main * msty,
				struct wb_style_base * base);
int get_bluetooth_fd(struct wb_context * ctx);
void * get_bluetooth_data(struct wb_context * ctx);
void bluetooth_get(struct wb_event * event, struct wb_context * ctx, void * data);
void bluetooth_render(struct wb_context * ctx, void * state);

static struct module_interface mod = {
	.module_name	= "bluetooth",
	.parse_sty		= bluetooth_parse_sty,
	.get_fd			= get_bluetooth_fd,
	.set_up			= get_bluetooth_data,
	.handle_event	= bluetooth_get,
	.emit_layout	= bluetooth_render,
	.clean_up		= NULL
};

struct bz_node {
	char name[64];
	char address[64];

	struct bz_node * next, * prev;
};

struct bz_state {
	sd_bus * bus;
	char name[64];
	char address[64];

	int initialized;
	int powered;
	int conn_count;
	struct bz_node * head;
};

struct module_interface * mod_init(){
	return &mod;
}

void
bluetooth_render(struct wb_context * ctx, void * state)
{

}

void
bluetooth_parse_sty(struct wb_config_setting * set, struct wb_style_main * msty,
				struct wb_style_base * base)
{
	
}

static int
read_prop_s(sd_bus_message * m, void * data, int length)
{
	const char * prop;
	int res = sd_bus_message_enter_container(m, SD_BUS_TYPE_VARIANT, "s");
	if (res < 0) {
		return res;
	}

	sd_bus_message_read_basic(m, SD_BUS_TYPE_STRING, &prop);
	strncpy(data, prop, length);

	sd_bus_message_exit_container(m);
	return 0;
}

static int
read_prop_b(sd_bus_message * m, void * data, int size)
{
	int res = sd_bus_message_enter_container(m, SD_BUS_TYPE_VARIANT, "b");
	if (res < 0) {
		return res;
	}

	sd_bus_message_read_basic(m, SD_BUS_TYPE_BOOLEAN, data);
	sd_bus_message_exit_container(m);
	return 0;
}

static int
read_adapter(sd_bus_message * m, struct bz_state * state)
{
	const char * address_prop = "Address";
	const char * name_prop = "Name";
	const char * powered_prop = "Powered";

	int res;
	while ((res = sd_bus_message_enter_container(m, SD_BUS_TYPE_DICT_ENTRY, "sv")) > 0) {
		if (res < 0) {
			return res;
		}
		const char * string;
		const char * contents;
		sd_bus_message_read_basic(m, SD_BUS_TYPE_STRING, &string);
		
		if (strcmp(string, address_prop) == 0) {
			read_prop_s(m, state->address, 64);
		} else if (strcmp(string, name_prop) == 0) {
			read_prop_s(m, state->name, 64);
		} else if (strcmp(string, powered_prop) == 0) {
			read_prop_b(m, &state->powered, sizeof(int));
		} else {
			sd_bus_message_skip(m, "v");
		}

		sd_bus_message_exit_container(m);
	}
}

static int
read_device(sd_bus_message * m, struct bz_state * state)
{
	struct bz_node * node = malloc(sizeof(struct bz_node));
	int paired = 0;

	const char * address_prop = "Address";
	const char * name_prop = "Name";
	const char * paired_prop = "Paired";

	int res;
	while ((res = sd_bus_message_enter_container(m, SD_BUS_TYPE_DICT_ENTRY, "sv")) > 0) {
		if (res < 0) {
			return res;
		}
		const char * string;

		sd_bus_message_read_basic(m, SD_BUS_TYPE_STRING, &string);

		if (strcmp(string, address_prop) == 0 && strcmp(string, "Address") != 0) {
			read_prop_s(m, node->address, 64);
		} else if (strcmp(string, name_prop) == 0) {
			read_prop_s(m, node->name, 64);
		} else if (strcmp(string, paired_prop) == 0) {
			read_prop_b(m, &paired, sizeof(int));
		} else {
			sd_bus_message_skip(m, "v");
		}

		sd_bus_message_exit_container(m);
	}

	if (paired) {
		DL_APPEND(state->head, node);
	} else {
		free(node);
	}
}



static int
read_interface(sd_bus_message * m, struct bz_state * state)
{
	const char * adapter = "org.bluez.Adapter1";
	const char * device = "org.bluez.Device1";

	int adapter_eq, device_eq;

	int res;
	while ((res = sd_bus_message_enter_container(m, 'e', "sa{sv}")) > 0) {
		if (res < 0) {
			return res;
		}

		const char * interface;
		sd_bus_message_read_basic(m, 's', &interface);
		LOG_INFO("%s\n", interface);

		adapter_eq = strncmp(interface, adapter, strlen(adapter));
		device_eq = strncmp(interface, device, strlen(device));

		if (adapter_eq && device_eq) {
			sd_bus_message_skip(m, "a{sv}");
			sd_bus_message_exit_container(m);
			continue;
		}

		res = sd_bus_message_enter_container(m, 'a', "{sv}");
		if (res < 0) {
			return res;
		} 

		if (!adapter_eq) {
			read_adapter(m, state);
		} else if (!device_eq) {
			read_device(m, state);
		} 

		sd_bus_message_exit_container(m);
		sd_bus_message_exit_container(m);
	}
}

/*
 * parse expected a{oa{sa{sv}}} message
 */
static int
get_managed_objects_cb(sd_bus_message * m, void * data, sd_bus_error * err)
{
	struct bz_state * state = data;
	state->initialized = 1;
	int res;

	res = sd_bus_message_enter_container(m, SD_BUS_TYPE_ARRAY, "{oa{sa{sv}}}");
	if (res < 0) {
		return res;
	}

	while ((res = sd_bus_message_enter_container(m, SD_BUS_TYPE_DICT_ENTRY, "oa{sa{sv}}")) > 0) {
		const char * path;
		sd_bus_message_read_basic(m, SD_BUS_TYPE_OBJECT_PATH, &path);

		const char * target = "/org/bluez/hci0";
		LOG_INFO("%s\n", path);
		if (strncmp(path, target, strlen(target)) != 0) {
			sd_bus_message_skip(m, "a{sa{sv}}");
			sd_bus_message_exit_container(m);
			continue;
		}

		res = sd_bus_message_enter_container(m, SD_BUS_TYPE_ARRAY, "{sa{sv}}");
		if (res < 0) {
			return res;
		} else if (res != 0) {
			read_interface(m, state);
		}
		
		sd_bus_message_exit_container(m);
		sd_bus_message_exit_container(m);
	}

	LOG_INFO("BLUEZ %s\n%s\n%d\n", state->name, state->address, state->conn_count);
}

/*
 * expected message oa{sa{sv}}
 */
static int
interface_add_cb(sd_bus_message * m, void * data, sd_bus_error * err)
{
	sd_bus_message_dump(m, NULL, SD_BUS_MESSAGE_DUMP_WITH_HEADER);
}

/*
 * expected message oas
 */
static int
interface_remove_cb(sd_bus_message * m, void * data, sd_bus_error * err)
{
	LOG_INFO("Bluez Removed Add\n");
}

/*
 * expected message sa{sv}as
 */
static int
prop_change_cb(sd_bus_message * m, void * data, sd_bus_error * ret)
{
	LOG_INFO("Bluez Prop Changed\n");
}

void
bluetooth_get(struct wb_event * event, struct wb_context * ctx, void * data)
{
	struct bz_state * state = data;

	while(sd_bus_process(state->bus, NULL) > 0);
}

void *
get_bluetooth_data(struct wb_context * ctx)
{
	struct bz_state * state = calloc(1, sizeof(struct bz_state));
	state->bus = mod.data;
	mod.data = NULL;

	sd_bus_call_method_async(
					state->bus,
					NULL,
					"org.bluez",
					"/",
					"org.freedesktop.DBus.ObjectManager",
					"GetManagedObjects",
					get_managed_objects_cb,
					state,
					NULL);

	sd_bus_match_signal(
					state->bus,
					NULL,
					NULL,
					"/",
					"org.freedesktop.DBus.ObjectManager",
					"InterfacesAdded",
					interface_add_cb,
					state);

	sd_bus_match_signal(
					state->bus,
					NULL,
					NULL,
					"/",
					"org.freedesktop.DBus.ObjectManager",
					"InterfacesRemoved",
					interface_remove_cb,
					state);

	sd_bus_match_signal(
					state->bus,
					NULL,
					NULL,
					"/org/bluez/hci0",
					"org.freedesktop.DBus.Properties",
					"PropertiesChanged",
					prop_change_cb,
					state);

	return state;
}

int
get_bluetooth_fd(struct wb_context * ctx)
{
	sd_bus * bus;
	sd_bus_default_system(&bus);
	mod.data = bus;

	return sd_bus_get_fd(bus);
}
