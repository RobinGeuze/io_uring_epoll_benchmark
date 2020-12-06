/* Minimal epoll/pipe benchmark for small messages */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/epoll.h>

int main() {

    /* Create a pipe with two fds (read, write) */
    int fds[2];
    int p = pipe(fds);
    int readfd = fds[0];
    int writefd = fds[1];

    /* Add them to an epoll fd */
    int epfd = epoll_create1(0);
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLOUT;
    epoll_ctl(epfd, EPOLL_CTL_ADD, readfd, &event);
    epoll_ctl(epfd, EPOLL_CTL_ADD, writefd, &event);

    /* Some shared static data */
    char buf[16];

    /* Begin iterating, starting by writing, then writing and reading */
    for (int i = 0; i < 1000000; i++) {

        /* Ask epoll for events */
        struct epoll_event events[16];
        int readyfd = epoll_wait(epfd, events, 16, -1);

        /* Handle the events by either writing or reading to the pipe */
        for (int j = 0; j < readyfd; j++) {
            if (events[j].events & EPOLLIN) {
                int r = read(readfd, buf, 6);
            }
            if (events[j].events & EPOLLOUT) {
                int w = write(writefd, "Hello!", 6);
            }
        }
    }
}