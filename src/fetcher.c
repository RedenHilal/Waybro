#include "fetcher.h"

void set_nonblock(int fd){
    int flag = fcntl(fd, F_GETFL, 0);
    if (flag < 0)
        ON_ERR("fcntl - nonblock")

    if (fcntl(fd, F_SETFL, flag | O_NONBLOCK) < 0)
        ON_ERR("fcntl 2 - nonblock")

    return;
}

void * sysclick_get(void* data){
    int inotify_fd = inotify_init1(IN_CLOEXEC);
    if (inotify_fd < 0) {
        perror("Error on Click Fetch ");
        exit(1);
    }

    return NULL;
}

int power_get(sd_bus_message * m, void * data, sd_bus_error * ret_error){
    int pipe = *(int *)data;
    
	static int file_fd = 0;
	static int battery_last = 0;

	if (file_fd == 0){
		file_fd = open(POWER_PATH, 0);
	}

    char buffer[4];
    
    int bytereads = read(file_fd, buffer, sizeof(buffer));
    lseek(file_fd, SEEK_SET, 0);
    buffer[bytereads] = 0;
    int battery_now = atoi(buffer);

    if(battery_now == battery_last) return 0;
    battery_last = battery_now;
    write(pipe, &(Event){POWER, BATTERY_STATUS, battery_now}, sizeof(Event));
    
    return 0;
}

int ac_get(sd_bus_message * m, void * data, sd_bus_error * ret_error){
	int pipe = *(int *) data;

	static int charge = -1;
	static int file_fd = 0;

	if (file_fd == 0){
		file_fd = open(CHARGE_PATH, 0);
	}
	
	char buffer[4];
    
    int bytereads = read(file_fd, buffer, sizeof(buffer));
    lseek(file_fd, SEEK_SET, 0);

    int charge_now = buffer[0] - 48;

	if (charge == charge_now){
		return 0;
	}
	
	charge = charge_now;
    write(pipe, &(Event){POWER,CHARGE_STATUS,charge}, sizeof(Event));
    
    return 0;
}


int get_power_fd(void * data){
	sd_bus * bus;
	sd_bus_slot * slot;
	sd_bus_default_system(&bus);

	sd_bus_match_signal(bus,
						&slot,
						NULL,
						"/org/freedesktop/UPower/devices/battery_BAT0",
						NULL,
						"PropertiesChanged",
						power_get,
						data);

	int fd = sd_bus_get_fd(bus);
   
    return fd;
}

int get_ac_fd(void * data){
	sd_bus * bus;
	sd_bus_slot * slot;
	sd_bus_default_system(&bus);

	sd_bus_match_signal(bus,
						&slot,
						NULL,
						"/org/freedesktop/UPower/devices/line_power_AC0",
						NULL,
						"PropertiesChanged",
						ac_get,
						data);

	int fd = sd_bus_get_fd(bus);

	return fd;
}

