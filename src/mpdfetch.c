#include "../include/fetcher.h"

static char * mpd_sock_path;
static char * mpd_dir_path;

static void init_path(){
    char * home_path = getenv("HOME");
    char * sock_path = "/.config/mpd/socket/mpd.sock";
    char * dir_path = "/.config/mpd/socket/";

    int sock_path_len = strlen(home_path) + strlen(sock_path);
    int dir_path_len = strlen(home_path) + strlen(dir_path);
 
    char * sock_path_dummy = malloc(sock_path_len + 1);
    snprintf(sock_path_dummy, sock_path_len + 1, "%s%s", home_path,sock_path);

    char * dir_path_dummy = malloc(dir_path_len + 1);
    snprintf(dir_path_dummy, dir_path_len + 1, "%s%s", home_path,dir_path);

    mpd_sock_path = sock_path_dummy;
    mpd_dir_path = dir_path_dummy;   
}

static int find_starting_utf(char * string, int n){
    while(n > 0 && (string[n] & 0xc0) == 0x80){
        n--;
    }
    return n;
}

static void get_curr_song(int fd, char * write_buffer){
    char buffer[512];
    int write_buff_len = 124;

    int res = write(fd,"currentsong\n",12);

    int bytereads = read(fd,buffer,sizeof(buffer));
    buffer[bytereads] = 0;

    char * title = strstr(buffer, "Title: ");
    char * artist = strstr(buffer, "Artist: ");

    if(!title || !artist) {
        snprintf(write_buffer, 124, "Unknown");
    }
    else {
        title +=7;
        artist += 8;

        char * title_end = strchr(title, '\n');
        char * artist_end = strchr(artist,'\n');
        
        if(title_end) *title_end = 0;
        if(artist_end) *artist_end = 0;
        
        int byte_printed = snprintf(write_buffer, write_buff_len, "%s - %s", title,artist);
        
        if(byte_printed >= write_buff_len){
            int valid_cut = find_starting_utf(write_buffer, write_buff_len - 1);
            write_buffer[valid_cut] = 0;
        }
    }
}

void *handle_idle(void * data){
    struct fd_object * object = data;
    char buffer[512];
    int bytereads = read(object->fd,buffer,sizeof(buffer));
    
    if(bytereads <= 0) return NULL;
    buffer[bytereads] = 0;


    if (!strncmp(buffer, "changed: player", 15)){
        
        get_curr_song(object->fd, object->data);
        
        write(object->pipe, &(Event){MPD,MPD_SENT,1,object->data},sizeof(Event));
        
        
        int res = write(object->fd,"idle\n",5);
        
        if (res < 0){
            write(object->pipe, &(Event){MPD,MPD_DOWN,1}, sizeof(Event));
            epoll_ctl(object->epfd, EPOLL_CTL_DEL, object->fd, NULL);
            close(object->fd);
            free(object);
        }

    }

    else return NULL;

    return NULL;

}


void * mpd_get(void * data){
    static int n = 0;
    printf("mpd_get triggered: %d\n", ++n);
    struct fd_object *object = data;
   
    struct sockaddr_un sock_addr;

    char drainbuff[256];
    read(object->fd,drainbuff,sizeof(drainbuff));

    int socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (socket_fd < 0) ON_ERR("Socket - Mpd");
    
    sock_addr.sun_family = AF_UNIX;
    strncpy(sock_addr.sun_path, mpd_sock_path, sizeof(sock_addr.sun_path) - 1);

    int res = connect(socket_fd, (struct sockaddr *)&sock_addr, sizeof(sock_addr));
    if (res == 0) {
        write(object->pipe, &(Event){MPD,MPD_UP,1}, sizeof(Event));
    }
    else {
        close(socket_fd);
        return NULL;
    }

    struct fd_object * socket_object = malloc(sizeof(struct fd_object)) ;
    char * object_data = malloc(124);
    memcpy(socket_object,object,sizeof(struct fd_object));
    
    socket_object->handler = handle_idle;
    socket_object->fd = socket_fd;
    socket_object->data = object_data;

    struct epoll_event event;
    event.events = EPOLLIN | EPOLLET;
    event.data.ptr = socket_object;
    if(epoll_ctl(object->epfd, EPOLL_CTL_ADD, socket_fd, &event) < 0){
        perror("hai");
    }

    res = read(socket_fd,drainbuff,sizeof(drainbuff));

    get_curr_song(socket_fd, object->data);
    write(object->pipe,&(Event){MPD,MPD_SENT,0,object->data},sizeof(Event));

    res = write(socket_fd,"idle\n",5);
    if(res < 0){
        epoll_ctl(object->epfd, EPOLL_CTL_DEL, socket_fd, NULL);
        close(socket_fd);
        free(socket_object);
        return NULL;
    }


    return NULL;
}


int get_mpd_fd(){
    init_path();

    int intfd = inotify_init1(IN_CLOEXEC);
    if (inotify_add_watch(intfd, mpd_dir_path, IN_CREATE ) < 0)
        ON_ERR("Inotify - mpd")

    set_nonblock(intfd);

    
    return intfd;
}

void mpd_fd_init(struct fd_object * object){
    

}
