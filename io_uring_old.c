/* Minimal io_uring/pipe benchmark for small messages */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include "liburing.h"

int main() {

    /* Create a pipe with two fds (read, write) */
    int fds[2];
    int p = pipe(fds);
    int readfd = fds[0];
    int writefd = fds[1];

    /* Create an io_uring */
    struct io_uring ring;
    int r = io_uring_queue_init(64, &ring, /*IORING_SETUP_SQPOLL*/ 0);

    printf("io_uring_queue_init: %d\n", r);

    /* Some shared static data */
    char buf[16] = {};

    /* Begin iterating, starting by writing, then writing and reading */
    for (int i = 0; i < 1/*000000*/; i++) {

        struct io_uring_sqe *sqe;
        
        /* Prepare a write */
        sqe = io_uring_get_sqe(&ring);
        io_uring_prep_write(sqe, writefd, "HELLU!", 6, 0);

        /* Prepare a read */
        sqe = io_uring_get_sqe(&ring);
        io_uring_prep_read(sqe, readfd, buf, 6, 0);

        // this one is the io_uring_enter syscall
        int submitted = io_uring_submit_and_wait(&ring, 1);

        // mark completions as "seen"
        for (int j = 0; j < submitted; j++) {
            struct io_uring_cqe *cqe;
            int ret = io_uring_wait_cqe(&ring, &cqe);

            printf("error: %d\n", cqe->res);

            io_uring_cqe_seen(&ring, cqe);
        }
    }

    printf("%s\n", buf);
}