#include <stdio.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <string.h>

#define MAX_EVENTS 5

int main() {
    int epoll_fd, event_count;
    struct epoll_event event, events[MAX_EVENTS];

    // 1. Create the epoll instance
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("Failed to create epoll file descriptor");
        return 1;
    }

    // 2. Prepare the event structure for stdin (File Descriptor 0)
    event.events = EPOLLIN; // Watch for input (Read)
    event.data.fd = 0;      // Monitor stdin

    // Add stdin to the epoll interest list
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, 0, &event) == -1) {
        perror("Failed to add file descriptor to epoll");
        close(epoll_fd);
        return 1;
    }
	event.events = EPOLLOUT;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, 0, &event) == -1) {
        perror("Failed to add file descriptor to epoll");
        close(epoll_fd);
        return 1;
    }

    printf("Waiting for input (Type something and press Enter)...\n");

    while (1) {
        // 3. Wait for events (timeout of -1 means wait indefinitely)
        event_count = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        
        if (event_count == -1) {
            perror("epoll_wait failed");
            break;
        }

        // 4. Handle events
        for (int i = 0; i < event_count; i++) {
            if (events[i].data.fd == 0) {
                char buffer[256];
                ssize_t bytes = read(0, buffer, sizeof(buffer) - 1);
                buffer[bytes] = '\0';
                
                printf("Epoll triggered! You entered: %s", buffer);

                // Exit if we type "quit"
                if (strncmp(buffer, "quit", 4) == 0) {
                    goto cleanup;
                }
            }
        }
    }

cleanup:
    close(epoll_fd);
    return 0;
}
