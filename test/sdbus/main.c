#include <systemd/sd-bus.h>
#include <stdio.h>
#include <stdlib.h>

static int
parse_sv(sd_bus_message * m)
{
	int res;
	while ((res = sd_bus_message_enter_container(m, SD_BUS_TYPE_DICT_ENTRY, "sv")) > 0) {
		if (res < 0) {
			return res;
		}
		const char * string;
		const char * contents;
		char key;
		sd_bus_message_read_basic(m, SD_BUS_TYPE_STRING, &string);

		res = sd_bus_message_peek_type(m, &key, &contents);
		if (res < 0) {
			return res;
		}

		res = sd_bus_message_enter_container(m, SD_BUS_TYPE_VARIANT, contents);
		if (res < 0) {
			return res;
		}
		if (key) {
			printf("		%s = %s\n", string, contents);
		} else {
			printf("		%s = %s\n", string, contents);
		}
		sd_bus_message_skip(m, contents);

		sd_bus_message_exit_container(m);
		sd_bus_message_exit_container(m);
	}

}

static int
parse_sa(sd_bus_message * m)
{
	int res;
	while ((res = sd_bus_message_enter_container(m, SD_BUS_TYPE_DICT_ENTRY, "sa{sv}")) > 0) {
		if (res < 0) {
			return res;
		}

		const char * string;
		sd_bus_message_read_basic(m, 's', &string);
		printf("	%s\n", string);

		res = sd_bus_message_enter_container(m, SD_BUS_TYPE_ARRAY, "{sv}");
		if (res < 0) {
			return res;
		} else if (res != 0) {
			parse_sv(m);
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
	int res;

	res = sd_bus_message_enter_container(m, SD_BUS_TYPE_ARRAY, "{oa{sa{sv}}}");
	if (res < 0) {
		return res;
	}

	while ((res = sd_bus_message_enter_container(m, SD_BUS_TYPE_DICT_ENTRY, "oa{sa{sv}}")) > 0) {
		const char * path;
		sd_bus_message_read_basic(m, SD_BUS_TYPE_OBJECT_PATH, &path);
		printf("%s\n", path);

		res = sd_bus_message_enter_container(m, SD_BUS_TYPE_ARRAY, "{sa{sv}}");
		if (res < 0) {
			return res;
		} else if (res != 0) {
			parse_sa(m);
		}
		sd_bus_message_exit_container(m);
		sd_bus_message_exit_container(m);
		
	}
	sd_bus_message_exit_container(m);

}

int
main()
{
	sd_bus * bus;
	sd_bus_default_system(&bus);
	sd_bus_message * m;
	sd_bus_error err;

	sd_bus_call_method(
				bus,
				"org.bluez",
				"/",
				"org.freedesktop.DBus.ObjectManager",
				"GetManagedObjects",
				&err,
				&m,
				"");

	get_managed_objects_cb(m, NULL, &err);
	return 0;
}
