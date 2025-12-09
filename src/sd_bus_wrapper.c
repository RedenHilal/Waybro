#include "fetcher.h"


void * sd_bus_handler(void * data){
	static sd_bus * bus = NULL;

	if(bus == NULL){
		sd_bus_default_system(&bus);
	}

	while(sd_bus_process(bus, NULL) > 0);

	return NULL;
}
