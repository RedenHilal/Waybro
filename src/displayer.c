#include "displayer.h"
#include "module.h"
#include "fetcher.h"
#include "style.h"
#include "core.h"
#include "poll.h"
#include "render.h"
#include "comm.h"

#include "macro.h"

#include <assert.h>

#define MAX_EVENTS 64


static void get_style(struct module_interface ** interfaces, int mod_count,
						struct wb_style_sec ** entries, struct wb_style_main * msty){

	struct wb_style_sec * sec = NULL;

    for (int i = 0; i < mod_count; i++){
		HASH_FIND_STR(*entries, interfaces[i]->module_name, sec);

        interfaces[i]->parse_sty(sec, msty);
		// clean up later
		sec = NULL;
    }
}

int main(){
    struct appstate appstate = {0};
    //sigaction(SIGSEGV, &sigact, NULL);

    struct wb_style_sec * cpn_entries = NULL;
	read_config(NULL, &cpn_entries);
    struct wb_style_main * m_style = translate_mstyle(&cpn_entries);

	int sec_count = HASH_COUNT(cpn_entries);
	int mod_found;

	struct module_interface ** interfaces = load_modules(cpn_entries, &mod_found);

	printf("hai %d\n", mod_found);
	if (!mod_found){
		printf("No Module Found, Exiting..\n");
		exit(0);
	}

    get_style(interfaces, mod_found, &cpn_entries, m_style);
	
	struct wb_render wrender = {
		.m_style = m_style,
		.appstate = &appstate
	};

    int pipes[2];
    struct wb_poll_event events[MAX_EVENTS];

    pthread_t threadID;

    struct wb_data dump[MAX_EVENTS];
    
    if (pipe(pipes) != 0) 
        ON_ERR("Pipe - main")
    
	struct module_context mod_ctx = {pipes[1], mod_found, &appstate, interfaces};
    
    setwayland(&appstate, &wrender);
    wl_display_dispatch_pending(appstate.display);

	int wlfd = wl_display_get_fd(appstate.display);

	struct wb_poll_fort * fort = wb_poll_create(O_CLOEXEC, 0);

	struct wb_poll_handle * pipe_handle = wb_poll_reg_events(fort, pipes[0],
													WB_EVENT_READ, NULL);
	struct wb_poll_handle * wlfd_handle = wb_poll_reg_events(fort, wlfd,
													WB_EVENT_READ, NULL);
    
    if (pthread_create(&threadID, NULL, mainpoll, &mod_ctx) != 0)
        ON_ERR("pthread cretae - displayer")
    

    while (1){

        int ready = wb_poll_wait_events(fort, events, MAX_EVENTS, -1);
		printf("ready %d\n");

        for(int i = 0; i < ready; i++){

            if (events[i].fd == wlfd) {
                wl_display_dispatch(appstate.display);
            }

			else {
                int byte_recvs = read(pipes[0], &dump[i], sizeof(struct wb_data));
                if (byte_recvs < 0) {
                    perror("Err on recv\n");
                    continue;
                }

				int id = dump[i].id;
				interfaces[id]->handle_update(&wrender, &dump[i]);
            }

        }

        wl_display_flush(appstate.display);
        
    }

    return 0;
}
