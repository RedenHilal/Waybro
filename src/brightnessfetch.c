#include "../include/fetcher.h"

void * brightness_get(void* data){
    struct fd_object *object = data;
    char buffer[124];
    char brightnessbuf[4];
    
    int file_monitor = open(BRIGHTNESS_PATH, O_RDONLY | IN_CLOEXEC);
    lseek(file_monitor,SEEK_SET,0);

    int bytereads;
    bytereads = read(object->fd, buffer,sizeof(buffer));
    
    read(file_monitor,brightnessbuf,4);
    brightnessbuf[3] = 0;

    int brightness = atoi(brightnessbuf);
    write(object->pipe , &(Event){BRIGHTNESS,1,(brightness * 100)/255}, sizeof(Event) );
    
    close(file_monitor);
    return NULL;
}

int get_brightness_fd(){
    int inotify_fd = inotify_init1(IN_CLOEXEC);
    if (inotify_fd < 0) {
        perror("Error on Brightness Fetch ");
        exit(1);
    }  

    if (inotify_add_watch(inotify_fd, BRIGHTNESS_PATH, IN_MODIFY) < 0){
        perror("Error on Watching power file system ");
        exit(1);
    }

    return inotify_fd;
}