#include "poll.h"
#include <stdio.h>

struct wb_poll_handle {
	struct wb_poll_fd_node node;
};

const int event_idx[WB_IDX_MAX] = {
	[WB_IDX_READ]	= WB_EVENT_READ,
	[WB_IDX_WRITE]	= WB_EVENT_WRITE,
	[WB_IDX_HUP]	= WB_EVENT_HUP
};

static int count_active_ev(struct wb_poll_fd_obj * obj){
	int count = 0;
	int * ev_count = obj->ev_count;
	
	for (int i = 0; i < WB_IDX_MAX; i++){
		if (ev_count[i] > 0)
			count |= event_idx[i];
	}

	return __builtin_popcount(count);
}


struct wb_poll_fort * wb_poll_create(int pflag, int wflag){
	int wfd;
	struct rlimit fsize;
	struct wb_poll_fort * fort = malloc(sizeof(struct wb_poll_fort));

	if (fort == NULL)
		goto fail;

	if (getrlimit(RLIMIT_NOFILE, &fsize) != 0)
		goto fail;

	fsize.rlim_cur = WB_POLL_FD_LIMIT;

	if (setrlimit(RLIMIT_NOFILE, &fsize) != 0)
		goto fail;

	int limit = fsize.rlim_cur;
	struct wb_poll_fd_obj * list = calloc(sizeof(struct wb_poll_fd_obj), limit);

	if (list == NULL)
		goto fail;

#if defined(A_EPOLL)
	wfd = epoll_create1(pflag);	

#elif defined(A_KQUEUE)
	wfd = kqueue1(pflag);

#endif
	if (wfd < 0)
		goto fail;

	if (wflag & WB_EVENT_EDGE)
		fort->edge = 1;
	else 
		fort->edge = 0;
	
	fort->wfd = wfd;
	fort->ev_obj_list = list;

	return fort;

fail:
	return NULL;
}

static int tl_wb_flags(int wevent){

	int events = 0;

#if defined(A_EPOLL)

	if (wevent & WB_EVENT_READ)
			events |= EPOLLIN;
	if (wevent & WB_EVENT_WRITE)
			events |= EPOLLOUT;
	if (wevent & WB_EVENT_EDGE)
			events |= EPOLLET;
	if (wevent & WB_EVENT_HUP)
			events |= EPOLLHUP;

#elif defined(A_KQUEUE)

#endif
	return events;
}

int tl_poll_flags(void * data){
	int wevent = 0;

#if defined(A_EPOLL)
	struct epoll_event * epevent = data;
	int event_mask = epevent->events;
	if (event_mask & EPOLLIN)
			wevent |= WB_EVENT_READ;
	if (event_mask & EPOLLOUT)
			wevent |= WB_EVENT_WRITE;
	if (event_mask & EPOLLHUP)
			wevent |= WB_EVENT_HUP;

#elif defined(A_KQUEUE)
	struct kevent * kev = data;
	int event_mask = kev->filter;
	if (event_mask == EVFILT_READ)
			wevent |= WB_EVENT_READ;
	if (event_mask == EVFILT_WRITE)
			wevent |= WB_EVENT_WRITE;
	if (kev->flags & EV_EOF)
			wevent |= WB_EVENT_HUP;
#endif
	return wevent;
}

#if defined(A_KQUEUE)
static int iterate_kevents(struct wb_poll_fd_node * node, int events){
	int res;
	struct kevent kevent;
	int edge = 0;

	int fd = node->fd;
	int kfd = node->fort->wfd;

	if (fort->edge)
		edge = EV_CLEAR;

	if (events & WB_EVENT_READ){
		EV_SET(&kevent, fd, EVFILT_READ, EV_ADD | edge, 0, 0, data);
		res = kevent(kfd, &kevent, 1, NULL, 0, NULL);
		if (res > 0)
				return res;
	}
	if (events & WB_EVENT_WRITE){
		EV_SET(&kevent, fd, EVFILT_WRITE, EV_ADD | edge, 0, 0, data);
		kevent(kfd, &kevent, 1, NULL, 0, NULL);
		if (res < 0)
				return res;
	}
	
	return 0;
}

static int iterate_rmv_kevents(struct wb_poll_fd_node * node, int events){
	struct kevent kev;
	int res;
	int fd = node->fd;
	int kfd = node->fort->wfd;

	if (events & WB_EVENT_READ){
		EV_SET(&kev, fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
		res = kevent(kfd, &kev, 1, NULL, 0, NULL);
		if (res < 0)
			return res;
	}
	if (events  & WB_EVENT_WRITE){
		EV_SET(&kev, fd, EVFILT_WRITE, EV_DELTE, 0, 0, NULL);
		res = kevent(kfd, &kev, 1, NULL, 0, NULL);
		if (res < 0)
			return res;
	}

	return 0;
}

#endif

static void insert_node(struct wb_poll_fd_node * node, struct wb_poll_fd_obj * obj){
	
	node->next = NULL;
	node->prev = obj->tail;

	if (obj->tail)
		obj->tail->next = node;

	if (obj->head == NULL)
		obj->head = node;
	
	obj->tail = node;
}

static void add_ev_count(struct wb_poll_fd_obj * obj, int wevent){
	if (wevent & WB_EVENT_READ)
		obj->ev_count[WB_IDX_READ]++;
	if (wevent & WB_EVENT_WRITE)
		obj->ev_count[WB_IDX_WRITE]++;
	if (wevent & WB_EVENT_HUP)
		obj->ev_count[WB_IDX_HUP]++;
}

static int get_active_event(struct wb_poll_fd_obj * obj){
	int mask = 0;
	if (obj->ev_count[WB_IDX_READ])
		mask |= WB_EVENT_READ;
	if (obj->ev_count[WB_IDX_WRITE])
		mask |= WB_EVENT_WRITE;
	if (obj->ev_count[WB_IDX_HUP])
		mask |= WB_EVENT_HUP;

	return mask;
}

static int modify_event(struct wb_poll_fd_node * node, int event, int event_cur){
	struct wb_poll_fort * fort = node->fort;
	int res;
#if defined(A_EPOLL)
	int event_mask = tl_wb_flags(event_cur);
	struct epoll_event epevent;

	epevent.events = event_mask;
	epevent.data.ptr = node->data;

	res = epoll_ctl(fort->wfd, EPOLL_CTL_MOD, node->fd, &epevent);
	if (res < 0)
			return -1;

#elif defined(A_KQUEUE)
	int kevent_del = event_cur & ~event;
	int kevent_add = event & ~event_cur;

	int del_count = __builtin_popcount(kevent_del);
	int add_count = __builtin_popcount(kevent_add);

	if (del_count){
		res = iterate_rmv_kevents(node, kevent_del);
		if (res < 0)
			return -1;
	}

	if (add_count){
		res = iterate_kevents(node, kevent_add);
		if (res < 0)
			return -1;
	}

#endif
	return 0;
}

struct wb_poll_handle * wb_poll_reg_events(struct wb_poll_fort * fort,
									int fd, int wevent, void * data){
	int res;
	struct wb_poll_fd_node * node = malloc(sizeof(struct wb_poll_fd_node));

	if (node == NULL)
		return NULL;

	node->fd = fd;
	node->data = data;
	node->fort = fort;
	node->events = wevent;

	struct wb_poll_fd_obj * obj = &fort->ev_obj_list[fd];
	int active_ev = get_active_event(obj);
	int ev_add = wevent & ~active_ev;

	insert_node(node, obj);
	struct wb_poll_handle * handle = (struct wb_poll_handle *) node;

	if (ev_add == 0)
		goto no_add;

#if defined(A_EPOLL)
	if (active_ev != 0) {
		res = modify_event(node, active_ev, active_ev | ev_add);
	} else {
		int events = tl_wb_flags(wevent);
		struct epoll_event epoll_event;

		epoll_event.events = wevent;

		epoll_event.data.ptr = obj;

		res = epoll_ctl(fort->wfd, EPOLL_CTL_ADD, fd, &epoll_event);
	}

#elif defined(A_KQUEUE)
	res = iterate_kevents(node, events);
#endif
	no_add:

	add_ev_count(obj, wevent);

	return handle;
}

static int dec_ev_count(struct wb_poll_fd_obj * obj, int wevent){
	int del = 0;
	int * ev_count = obj->ev_count;

	if (wevent & WB_EVENT_READ){
		if (ev_count[WB_IDX_READ] <= 0)
			return -1;
		ev_count[WB_IDX_READ]--;
		if (ev_count[WB_IDX_READ] == 0)
			del |= WB_EVENT_READ;
	}
	if (wevent & WB_EVENT_WRITE){
		if (ev_count[WB_IDX_WRITE] <= 0)
			return -1;
		ev_count[WB_IDX_WRITE]--;
		if (ev_count[WB_IDX_WRITE] == 0)
			del |= WB_EVENT_WRITE;
	}
	if (wevent & WB_EVENT_HUP){
		if (ev_count[WB_IDX_HUP] <= 0)
			return -1;
		ev_count[WB_IDX_HUP]--;
		if (ev_count[WB_IDX_HUP] == 0)
			del |= WB_EVENT_HUP;
	}

	return del;
}


int wb_poll_rmv_events(struct wb_poll_handle * handle){
	int res = 0;
	struct wb_poll_fd_node * node = &handle->node;
	struct wb_poll_fd_obj * obj = &node->fort->ev_obj_list[node->fd];
	int event = get_active_event(obj);

	int ev_del = dec_ev_count(obj, node->events);
	int ev_count = count_active_ev(obj);

	// active event after deletion
	int ev_cur = event ^ ev_del;
	int del_count = __builtin_popcount(ev_del);

	if (ev_del < 0)
		return -1;

	if (ev_count == 0){
#if defined(A_EPOLL)
		res = epoll_ctl(node->fort->wfd, EPOLL_CTL_DEL, node->fd, NULL);

#elif defined(A_KQUEUE)
		res = itereate_rmv_kevents(node, wevent);
#endif
	} else if (del_count > 0) {
		res = modify_event(node, event, ev_cur);
	}

	if (node->prev)
		node->prev->next = node->next;
	else 
		obj->head = node->next;

	if (obj->tail == node)
		obj->tail = node->prev;

	free(handle);

	return 0;
}

static int count_node(struct wb_poll_fd_obj * obj, int wevent){
	int count = 0;
	for (int i = 0; i < WB_IDX_MAX; i++){
		if (event_idx[i] & wevent){
			count += obj->ev_count[i];
		}
	}
	return count;
}

int wb_poll_wait_events(struct wb_poll_fort * fort, struct wb_poll_event * events,
						int nevents, int timeouts){

	int ev_count;
	int wevent;
	int fd;

#if defined(A_EPOLL)
	struct epoll_event e_events[nevents];
	ev_count = epoll_wait(fort->wfd, e_events, nevents, timeouts);

	for (int i = 0; i < ev_count; i++){
		struct wb_poll_fd_obj * obj = e_events[i].data.ptr;

		fd = obj->head->fd;
		wevent = tl_poll_flags(&e_events[i]);

		events[i].ev_mask = wevent;
		events[i].fd = fd;
		events[i].count = count_node(obj, wevent);
		events[i].hcount = 0;
		events[i].handle = (struct wb_poll_handle *) obj->head;
	}
	
#elif defined(A_KQUEUE)
	struct kevent k_events[nevents];
	struct timespec ktimeout= {.tv_sec = timeouts .tv_nsec = 0};
	struct timespec * ktimespec = timeouts < 0 ? NULL : &ktimeout;

	ev_count = kevent(fort->wfd, NULL, 0, k_events, nevents, ktimespec);

	for (int i = 0; i < ev_count; i++){
		struct wb_poll_fd_obj * obj = k_events[i].udata;

		fd = k_events[i].ident;
		wevent = tl_poll_flags(&k_events[i]);

		events[i].ev_mask = wevent;
		events[i].fd = fd;
		events[i].count = count_node(obj, wevent):
		events[i].hcount = 0;
		events[i].handle = (struct wb_poll_handle *) obj->head;
	}
#endif
	
	return ev_count;
}

void * wb_poll_reap_event(struct wb_poll_fort * fort, struct wb_poll_event * event){

	if (event->hcount >= event->count){
		return NULL;
	}
	
	while (event->handle && !(event->handle->node.events & event->ev_mask)){
		event->handle = (struct wb_poll_handle *) event->handle->node.next;
	}

	if (event->handle == NULL){
		return NULL;
	}

	event->hcount++;
	void * data = event->handle->node.data;
	event->handle = (struct wb_poll_handle *) event->handle->node.next;
	return data;
	
}

void * wb_poll_data_from_handle(struct wb_poll_handle * handle){
	return handle->node.data;
}
