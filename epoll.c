/* Minimal epoll/pipe benchmark for small messages */

#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/epoll.h>

// a vector to keep track of the pipe? no, a struct attached to both ends
struct pipe_data {
    size_t written;
    size_t read;
    int readfd, writefd;
};

#define BUF_SIZE 1024

int pipes[1000];

void register_pipe(int epfd, int index) {
    struct pipe_data *pd = malloc(sizeof(struct pipe_data));

    /* Create a pipe with two fds (read, write) */
    int fds[2];
    int p = pipe2(fds, O_NONBLOCK);
    int readfd = fds[0];
    int writefd = fds[1];

    //int size = BUF_SIZE;
    //int ok = fcntl(readfd, F_SETPIPE_SZ, size);
    //fcntl(writefd, F_SETPIPE_SZ, size);

    //printf("ok: %d\n", ok);

    pd->readfd = readfd;
    pd->writefd = writefd;
    pd->written = 0;
    pd->read = 0;

    pipes[index * 2 + 1] = writefd;

    /* Add them to an epoll fd */
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.ptr = pd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, readfd, &event);

    // we write without polling for writable (fast path)
    //event.events = EPOLLOUT;
    //event.data.ptr = pd;
    //epoll_ctl(epfd, EPOLL_CTL_ADD, writefd, &event);
}

#include <time.h>
#include <string.h>

int main(int argc, char **argv) {
    int num_pipes = 0;
    sscanf(argv[1], "%d", &num_pipes);

    printf("Pipes: %d\n", num_pipes);

    int epfd = epoll_create1(0);
    for (int i = 0; i < num_pipes; i++) {
        register_pipe(epfd, i);
    }

    /* Some shared static data */
    char *buf = malloc(BUF_SIZE);
    memset(buf, 'A', BUF_SIZE);
    struct epoll_event events[2000];

    clock_t start = clock();

    /* Begin iterating, starting by writing, then writing and reading */
    for (int i = 0; i < 1000000; i++) {

        /* First we write a message to all pipes */
        for (int j = 0; j < num_pipes; j++) {

            int writefd = pipes[j * 2 + 1];

            int w = write(/*pd->*/writefd, /*"Hello!"*/ buf, /*6*/ BUF_SIZE);
            if (w != BUF_SIZE) {
                printf("write mismatch: %d\n", w);
                exit(1);
            }
        }

        /* Ask epoll for events */
        int readyfd = epoll_wait(epfd, events, 2000, -1);

        /* Handle the events by either writing or reading to the pipe */
        for (int j = 0; j < readyfd; j++) {

            struct pipe_data *pd = events[j].data.ptr;

            if (events[j].events & EPOLLIN) {
                int r = read(pd->readfd, buf, /*6*/ BUF_SIZE);
                if (r != BUF_SIZE) {
                    printf("asdasdsa\n");
                    exit(1);
                }
                pd->read += r;
            }
            /*if (events[j].events & EPOLLOUT) {
                int w = write(pd->writefd, "Hello!", 6);
                if (w != 6) {
                    printf("asdasdsa\n");
                    exit(1);
                }
                pd->written += w;
            }*/
        }
    }


    clock_t end = clock();

    double cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;

    printf("Time: %f\n", cpu_time_used);
}