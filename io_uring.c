/* Minimal io_uring/pipe benchmark for small messages */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include "liburing.h"

int writefd, readfd;

    /* Some shared static data */
    char buf[16] = {};

void io_uring_minimal_nop_loop(struct io_uring *ring) {

    struct io_uring_sqe *sqe;
    struct io_uring_cqe *cqes[16];
    //struct io_uring_cqe *cqe;

    for (int i = 0; i < 1000000; i++) {
        //sqe = io_uring_get_sqe(ring);
        //io_uring_prep_nop(sqe);

        /* Prepare a write */
        sqe = io_uring_get_sqe(ring);
        io_uring_prep_write(sqe, writefd, "HELLU!", 6, 0);

        /* Prepare a read */
        sqe = io_uring_get_sqe(ring);
        io_uring_prep_read(sqe, readfd, buf, 6, 0);

        io_uring_submit(ring);

        // io_uring_wait_cqe(ring, &cqe); -- very slow!

        /* Poll for completions */
        while (2 != io_uring_peek_batch_cqe(ring, cqes, 16));
        io_uring_cqe_seen(ring, cqes[0]);
        io_uring_cqe_seen(ring, cqes[1]);
    }

    printf("%s\n", buf);
}

int main() {

    /* Create a pipe with two fds (read, write) */
    int fds[2];
    int p = pipe(fds);
    /*int */readfd = fds[0];
    /*int */writefd = fds[1];

    /* Create an io_uring */
    struct io_uring ring;
    int r = io_uring_queue_init(64, &ring, IORING_SETUP_SQPOLL | IORING_SETUP_SQ_AFF /*0*/); //without this, we get 2 seconds and io_uring_enter syscalls, with we get 0.81 sec

    printf("io_uring_queue_init: %d\n", r);


    /* Start the most minimal NOP-loop possible */
    io_uring_minimal_nop_loop(&ring);

    return 0;


    /* Begin iterating, starting by writing, then writing and reading */
    for (int i = 0; i < 1000000; i++) {

        struct io_uring_sqe *sqe;
        
        /* It is actually enough with a NOP to trigger the slowdown */
        io_uring_prep_nop(sqe);

        /* Prepare a write */
        //sqe = io_uring_get_sqe(&ring);
        //io_uring_prep_write(sqe, writefd, "HELLU!", 6, 0);

        /* Prepare a read */
        //sqe = io_uring_get_sqe(&ring);
        //io_uring_prep_read(sqe, readfd, buf, 6, 0);

        io_uring_submit(&ring);

        // this one is the io_uring_enter syscall
        //int submitted = io_uring_submit_and_wait(&ring, 1);

        // mark completions as "seen"
        //for (int j = 0; j < submitted; j++) {
            struct io_uring_cqe *cqe;
            int ret = io_uring_wait_cqe(&ring, &cqe);

            io_uring_cqe_seen(&ring, cqe);
        //}
    }
}