/* Minimal epoll/pipe benchmark for small messages */

#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <time.h>
#include <string.h>

#define MAX_PIPES 10000
#define BUF_SIZE 1024

int *pipes;

/* Per poll data */
struct pipe_data {
    size_t written;
    size_t read;
    int readfd, writefd;
};

void register_pipe(int epfd, int index) {
    struct pipe_data *pd = malloc(sizeof(struct pipe_data));

    /* Create a pipe with two fds (read, write) */
    int fds[2];
    int p = pipe2(fds, O_NONBLOCK);
    int readfd = fds[0];
    int writefd = fds[1];

    pd->readfd = readfd;
    pd->writefd = writefd;
    pd->written = 0;
    pd->read = 0;

    pipes[index * 2 + 1] = writefd;

    /* We only need to poll for readable as writing without is a known fast path */
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.ptr = pd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, readfd, &event);
}

int main(int argc, char **argv) {

    pipes = malloc(sizeof(int) * MAX_PIPES * 2);

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

    /* This is similar to MAX_BATCH in io_uring */
    struct epoll_event events[2000];

    /* Start time */
    clock_t start = clock();

    /* Begin iterating, starting by writing, then writing and reading */
    for (int i = 0; i < 10000; i++) {

        /* First we write a message to all pipes */
        for (int j = 0; j < num_pipes; j++) {

            /* Pipes are ordered as readfd, writefd */
            int writefd = pipes[j * 2 + 1];

            /* Write data using syscall */
            int written = write(writefd, buf, BUF_SIZE);
            if (written != BUF_SIZE) {
                printf("Failed writing data in fast path: %d\n", written);
                return 0;
            }
        }

        /* We expect to read everything before we continue iterating */
        int remaining_reads = num_pipes;

        while (remaining_reads) {
            /* Ask epoll for events in batches */
            int readyfd = epoll_wait(epfd, events, 2000, -1);

            /* Handle the events by either writing or reading to the pipe */
            for (int j = 0; j < readyfd; j++) {
                /* Get associated per poll data */
                struct pipe_data *pd = events[j].data.ptr;

                /* If we can read, we read */
                if (events[j].events & EPOLLIN) {
                    int r = read(pd->readfd, buf, BUF_SIZE);
                    if (r != BUF_SIZE) {
                        printf("Read wrong amounts of data!\n");
                        return 0;
                    }
                    pd->read += r;
                }
            }

            /* We only keep readable fds so this is fine */
            remaining_reads -= readyfd;
        }
    }

    /* Print time */
    clock_t end = clock();
    double cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("Time: %f\n", cpu_time_used);

    /* Append this run */
    FILE *runs = fopen("epoll_runs", "a");
    fprintf(runs, "%f\n", cpu_time_used);
    fclose(runs);
}