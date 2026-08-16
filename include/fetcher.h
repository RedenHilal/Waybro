#include "displayer.h"

#ifndef FETCHER_H
#define FETCHER_H

#define NET_PATH "/sys/class/net/wlp1s0/operstate"

#define MEM_PATH "/proc/meminfo"


// get fd(s)

// fetcher function

// dirty init here


// main poling

void *mainpoll(void * data);


// handler function


// starter function 


// clean up function


// misc
void resources_init(void *);
void set_nonblock(int fd);
void handle_segv(int);
void handle_sigttou(int);
void proc_reg(int);


#endif
