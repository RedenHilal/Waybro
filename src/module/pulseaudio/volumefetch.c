#include "fetcher.h" 
#include <pulse/context.h>
#include <pulse/mainloop.h>
#include <pulse/subscribe.h>
#include <pulse/def.h>
#include <pulse/volume.h>
#include <pulse/introspect.h>

int get_volume_fd(void * data);
void volume_get(struct epoll_event * event);
void get_volume_data(struct fd_object * object);

struct pa_state{
	pa_context * context;
	pa_mainloop * ml;
	pa_mainloop_api * mla;

	int wpipe;
};

static const struct module_interface mod = {
	.module_name	= "pulseaudio",
	.parse_sty		= NULL,
	.get_fd			= get_volume_fd,
	.set_up			= get_volume_data,
	.handle_event	= volume_get,
	.handle_update	= NULL,
	.clean_up		= NULL
};

static struct pa_state * ps;
static pthread_t tid;
static int retval;

struct module_interface * mod_init(){
		return &mod;
}

static void pa_sink_info_cb(pa_context * c, const pa_sink_info * i, int eol, void * data){
	if(eol){
		return;
	}
	pa_volume_t vol_raw = pa_cvolume_avg(&i->volume);
	int vol_now = (vol_raw * 100) / PA_VOLUME_NORM;


	write(ps->wpipe, &vol_now, sizeof(int));
	printf("triggered sink vol: %d\n", vol_now);
}

static void pa_server_info_cb(pa_context * c, const pa_server_info * i, void * data){
	const char * default_sink = i->default_sink_name;

	printf("triggered server\n");
	pa_operation * op = pa_context_get_sink_info_by_name(c, default_sink, pa_sink_info_cb, data);	

}

static void pa_subscribe_cb(pa_context * c, pa_subscription_event_type_t t, uint32_t idx, void * data){

	t = t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK;
	printf("triggered subscribe\n");
	switch (t){
			case PA_SUBSCRIPTION_EVENT_SINK:
					pa_context_get_server_info(c, pa_server_info_cb, data);
	}
}

static void pa_success_cb(pa_context * c, int success, void * data){

	pa_context_set_subscribe_callback(c, pa_subscribe_cb, NULL);	
	printf("triggered success\n");
}

static void pa_state_cb(pa_context * c, void * data){
	pa_context_state_t state = pa_context_get_state(c);

	printf("triggered state %d\n", state);
	switch (state){
		case PA_CONTEXT_READY:		
				pa_context_get_server_info(c, pa_server_info_cb, data);
				pa_context_subscribe(c, PA_SUBSCRIPTION_MASK_SINK, pa_success_cb, NULL);
				printf("triggered successs\n");
				break;
		case PA_CONTEXT_FAILED:
				ON_ERR("pa_failed");
		default:
				// idk
	}
}

static void * pa_listener(void * data){
	printf("gai\n");
	pa_mainloop * ml = pa_mainloop_new();    
	pa_mainloop_api * mla = pa_mainloop_get_api(ml);
	pa_context * pactx = pa_context_new(mla, "waybro");

	ps->context = pactx;
	ps->ml = ml;
	ps->mla = mla;

	pa_context_set_state_callback(pactx, pa_state_cb, NULL);
	if(pa_context_connect(pactx, NULL, 0, NULL) < 0) 
			ON_ERR("pa_connect")
	if(pa_mainloop_run(ps->ml, &retval) < 0)
			ON_ERR("pa ml run thread")

	return NULL;
}



int get_volume_fd(int pipe){
    
	int pipes[2];
	if(pipe(pipes) != 0)
			ON_ERR("pipe pulse")

	ps = malloc(sizeof(struct pa_state));
	ps->wpipe = pipes[1];

	return pipes[0];
}

void get_volume_data(struct wb_context * context){
	pthread_create(&tid, NULL, pa_listener, NULL);
}


void volume_get(struct wb_event * event, struct wb_context * ctx){
	struct fd_object * object = event.data.ptr;
	int vol;

	read(object->fd, &vol, sizeof(int));
	write(object->pipe, &(Event){VOLUME, 0, vol}, sizeof(Event));
    return NULL;
}
