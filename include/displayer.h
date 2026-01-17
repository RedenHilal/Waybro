#ifndef DISPLAYER_H
#define DISPLAYER_H

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>

#include <sys/epoll.h>
#include <sys/inotify.h>
#include <sys/timerfd.h>

#include <fcntl.h>
#include <pthread.h>
#include <systemd/sd-bus.h>

#include <time.h>
#include <signal.h>
#include <errno.h>
#include <assert.h>

#include <sys/stat.h>
#include <sys/mman.h>


#define MAKS_EVENT 64
#define DATA_COUNT 11

#define INT_BITS (sizeof(int) * 8)


struct fd_object {
    int fd;
	int mod_id;
    struct module_interface * mod_int; 
};


struct proc_register {
    int proc[DATA_COUNT];
    int count;
};



#endif
