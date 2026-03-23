#include "displayer.h"
#include "module.h"
#include "fetcher.h"
#include "style.h"
#include "core.h"
#include "poll.h"
#include "render.h"
#include "comm.h"
#include "layout.h"
#include "config.h"
#include "layout.h"
#include "widget.h"

#include "clay.h"
#include "macro.h"

#include <assert.h>

#include <sys/epoll.h>

#define MAX_EVENTS 64


static void
parse_mod_stys(struct module_interface ** interfaces, int mod_count,
			struct wb_config_setting ** mod_sets, struct wb_style_main * msty,
			struct wb_layout * layout)
{

    for (int i = 0; i < mod_count; i++){

		struct wb_style_base * base = malloc(sizeof(struct wb_style_base));
		if (base == NULL)
			ON_ERR("allocation failed for style_base")

		parse_layout_hint(layout, mod_sets[i], i);
		
		wb_style_get_base(base, mod_sets[i], msty);
        interfaces[i]->parse_sty(mod_sets[i], msty, base);
		// clean up later
    }
}


/*
 * TODO
 * Font config
 */
static void
widget_init(struct wb_render * wrender, struct module_context * mod_ctx)
{
	struct wb_style_main * msty = wrender->m_style;
	wb_layout_arena_allocate(msty->width, msty->height);
	char * fonts[] = {
		"Times New Roman"
	};

	wb_layout_font_init(fonts);
}

static void
mutex_init(struct module_context * mod_ctx)
{
	int count = mod_ctx->module_count;
	pthread_mutex_t * mutexes = malloc(sizeof(pthread_mutex_t) * count);
	if (mutexes == NULL)
		ON_ERR("Mutex Allocation Failed")

	for (int i = 0; i < count; i++){
		int res = pthread_mutex_init(&mutexes[i], NULL);
		if (res < 0)
			ON_ERR("Mutex Init Failed")
	}

	mod_ctx->mutexes = mutexes;
}

static void
states_init(struct module_context * mod_ctx)
{
	int count = mod_ctx->module_count;
	void ** states = malloc(sizeof(void *) * count);

	if (states == NULL)
		ON_ERR("States Allocation failed")
	
	mod_ctx->states = states;
}

int main()
{
    struct wb_appstate appstate = {0};
	struct wb_widget_interest_list ilist = {0};
    //sigaction(SIGSEGV, &sigact, NULL);

	char * paths[WB_CONFIG_PATHS_NUM];
	wb_config_get_paths(paths);

	struct wb_config * cfg = wb_config_init(paths);
	struct wb_config_setting * gen_set = wb_config_get_setting(cfg, "general");

    struct wb_style_main * m_style = wb_style_get_main(gen_set);
	struct wb_config_setting * modules = wb_config_get_setting(cfg, "modules");

	int mod_count = wb_config_s_length(modules);
	struct wb_config_setting * mod_sets[mod_count];

	struct module_interface ** interfaces = load_modules(modules, &mod_count,
					mod_sets, &appstate);
	struct wb_layout layout;

    parse_mod_stys(interfaces, mod_count, mod_sets, m_style, &layout);

    int pipes[2];
    struct wb_poll_event events[MAX_EVENTS];

    pthread_t threadID;
	sem_t sem;
	sem_init(&sem, 0, 0);

    if (pipe(pipes) != 0) 
        ON_ERR("Pipe - main")
    
	struct wb_context wb_ctx = {
			.appstate = &appstate,
			.ilist = &ilist,
			.layout = &layout
	};

	struct module_context mod_ctx = {
			.pipe = pipes[1],
			.module_count = mod_count,
			.interfaces = interfaces,
			.sem = &sem,
			.sets = mod_sets,
			.ctx = &wb_ctx
	};

	struct wb_render wrender = {
			.m_style = m_style
	};

	mutex_init(&mod_ctx);
	states_init(&mod_ctx);

    setwayland(&appstate, &wrender);
    wl_display_dispatch_pending(appstate.display);

	int wlfd = wl_display_get_fd(appstate.display);

	struct wb_poll_fort * fort = wb_poll_create(O_CLOEXEC, WB_EVENT_EDGE);

	struct wb_poll_handle * pipe_handle = wb_poll_reg_events(fort, pipes[0],
													WB_EVENT_READ, NULL);
	struct wb_poll_handle * wlfd_handle = wb_poll_reg_events(fort, wlfd,
													WB_EVENT_READ, NULL);
    
	//widget_init(&wrender);

    if (pthread_create(&threadID, NULL, mainpoll, &mod_ctx) != 0)
        ON_ERR("pthread cretae - displayer")
   
	/*
	 * wait for modules set_up to be done
	 */
	sem_wait(&sem);
	char dump[1024];


    while (1){

		printf("waiting for display events\n");
        int ready = wb_poll_wait_events(fort, events, MAX_EVENTS, -1);
		printf("ready %d\n", ready);

        for(int i = 0; i < ready; i++){

            if (events[i].fd == wlfd) {
                wl_display_dispatch(appstate.display);
            }

			else {
                int byte_recvs = read(pipes[0], dump, sizeof(dump));
                if (byte_recvs < 0) {
                    perror("Err on recv\n");
                    continue;
                }
				/*
				 * render bar
				 */

            }

        }

        wl_display_flush(appstate.display);
        
    }

    return 0;
}
