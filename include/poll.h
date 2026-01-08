#ifndef WBRO_POLL
#define WBRO_POLL

#if defined(__linux__)
	#define A_EPOLL
	#include <sys/epoll.h> 
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
	#define A_KQUEUE
	#include <sys/types.h>
	#include <sys/event.h>
	#include <sys/time.h>
	#include <fcntl.h>
#else
	#define A_POLL
	//fk poll, performance supremacy
#endif

#include <sys/resource.h>
#include <stdlib.h>

#ifndef WB_POLL_FD_LIMIT
#define WB_POLL_FD_LIMIT 1024
#endif

enum {
	WB_EVENT_READ = 1 << 0,
	WB_EVENT_WRITE = 1 << 1,
	WB_EVENT_HUP = 1 << 2,
	WB_EVENT_EDGE = 1 << 3
};

enum {
	WB_IDX_READ,
	WB_IDX_WRITE,
	WB_IDX_HUP,
	WB_IDX_MAX
};

struct wb_poll_fd_node;
struct wb_poll_fd_obj;

//opaque handler for userspace
struct wb_poll_handle;

struct wb_poll_fort {
	int wfd;
	int edge;
	struct wb_poll_fd_obj * ev_obj_list;
};

struct wb_poll_fd_obj {
	struct wb_poll_fd_node * head;
	struct wb_poll_fd_node * tail;
	int ev_count[WB_IDX_MAX];
};

struct wb_poll_fd_node {
	struct wb_poll_fort * fort;
	void * data;
	int fd;
	int events;
	struct wb_poll_fd_node * prev;
	struct wb_poll_fd_node * next;
};


struct wb_poll_event {
	int fd;
	int ev_mask;
	int count;
	int hcount;
	struct wb_poll_handle * handle;
};

struct wb_poll_fort * wb_poll_create(int pflag, int wflag);

struct wb_poll_handle * wb_poll_reg_events(struct wb_poll_fort * fort,
									int fd, int wevent, void * data);

int wb_poll_rmv_events(struct wb_poll_handle * handle);

int wb_poll_wait_events(struct wb_poll_fort * fort, struct wb_poll_event * event,
						int nevents, int timeouts);

// reap data from an event, will return registered data
// until every registered data read.
// at that point future reap will return NULL;
void * wb_poll_reap_event(struct wb_poll_fort * fort, struct wb_poll_event * event);


// global idx table
extern const int event_idx[WB_IDX_MAX];

// test purpose
int tl_poll_flags(void * data);

#endif
