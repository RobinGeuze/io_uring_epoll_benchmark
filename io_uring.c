/* Minimal io_uring/pipe benchmark for small messages */

#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include "liburing.h"

#include <stdlib.h>

#define BUF_SIZE 1024

//int writefd, readfd;

    // vektor av read,write ints
    int pipes[100];

    /* Some shared static data */
    char buf[16] = {};

    int num_pipes = 0;

        struct iovec buffers[64];

        #include <errno.h>
        #include <string.h>

void io_uring_minimal_nop_loop(struct io_uring *ring) {

    struct io_uring_sqe *sqe;
    struct io_uring_cqe *cqes[100];
    //struct io_uring_cqe *cqe;

    for (int i = 0; i < 1000000; i++) {

        for (int j = 0; j < num_pipes; j++) {

            int readfd = pipes[j * 2];
            int writefd = pipes[j * 2 + 1];

            

            /* Prepare a write */
            sqe = io_uring_get_sqe(ring);
            //io_uring_prep_write(sqe, writefd, "HELLU!", 6, 0);
            io_uring_prep_write_fixed(sqe, /*writefd*/ j * 2 + 1, buffers[j * 2 + 1].iov_base, /*6*/ BUF_SIZE, 0, j * 2 + 1);



            //io_uring_prep_write(sqe, 1, "HELLU!", /*BUF_SIZE*/6, 0);
            sqe->flags |= IOSQE_FIXED_FILE;

            /* Prepare a read */
            sqe = io_uring_get_sqe(ring);
            io_uring_prep_read_fixed(sqe, /*readfd*/ j * 2, buffers[j * 2].iov_base, /*6 */ BUF_SIZE, 0, j * 2);
            //io_uring_prep_read(sqe, 0, buf, 6, 0);
             sqe->flags |= IOSQE_FIXED_FILE;
        }


        io_uring_submit(ring);
        int submitted = 2 * num_pipes;

        // this one is the io_uring_enter syscall
        //io_uring_submit(ring);
        //int submitted = /*2 * num_pipes;*/ io_uring_submit_and_wait(ring, /*2 **/ num_pipes);

        // mark completions as "seen"
        for (int j = 0; j < submitted; j++) {
            struct io_uring_cqe *cqe;
            int ret;
            again:
            ret = io_uring_peek_cqe(ring, &cqe);

            if (ret != 0) {
                goto again;
            }

            if (ret != 0 || cqe->res < 0) {
                printf("ASYNC FAILURE!\n");
                printf("%s\n", strerror(-cqe->res));
                exit(0);
            }

            if (cqe->res != BUF_SIZE) {
                printf("MISMATCHING READ: %d\n", cqe->res);
                exit(0);
            }

            //printf("ok!\n");


            //printf("ret: %d\n", ret);

            //printf("%s\n", strerror(-cqe->res));

            io_uring_cqe_seen(ring, cqe);
        }

        //printf("%s\n", buf);

        continue;


        //io_uring_submit(ring);

        io_uring_submit_and_wait(ring, 1);

        //io_uring_wait_cqe(ring, &cqe); -- very slow!

        /* Poll for completions */
        int remaining_completions = num_pipes /** 2*/;

        while (remaining_completions) {
            int completions = io_uring_peek_batch_cqe(ring, cqes, 100);

            if (completions < 0) {
                printf("not done yet!\n");
                continue;
            }

            for (int j = 0; j < completions; j++) {

                printf("%s\n", strerror(-cqes[j]->res));

                printf("ERRNO completion: %d\n", cqes[j]->res);

                io_uring_cqe_seen(ring, cqes[j]);
            }

            remaining_completions -= completions;
        }
    }
}

#include <time.h>

#include <stdlib.h>
#include <string.h>

//#include <linux/io_uring.h>

int main(int argc, char **argv) {

    //num_pipes = 0;
    sscanf(argv[1], "%d", &num_pipes);

    printf("Pipes: %d\n", num_pipes);
    for (int i = 0; i < num_pipes; i++) {
        if (pipe2(&pipes[i * 2], O_NONBLOCK | O_CLOEXEC)) {
            printf("ERROR! pipes!\n");
        }

        //int readfd = pipes[i * 2];
        //int writefd = pipes[i * 2 + 1];

        //int size = BUF_SIZE;
        //int ok = fcntl(readfd, F_SETPIPE_SZ, size);
        //fcntl(writefd, F_SETPIPE_SZ, size);

        //printf("ok: %d\n", ok);
    }



    /* Create an io_uring */
    struct io_uring ring;
    int r = io_uring_queue_init(64, &ring, IORING_SETUP_SQPOLL/* | IORING_SETUP_SQ_AFF*/ /*IORING_SETUP_IOPOLL*/ /*0*/ /*0*/); //without this, we get 2 seconds and io_uring_enter syscalls, with we get 0.81 sec

    if (r) {
        printf("ERROR!\n");
        return 0;
    }

        // needed for sqpoll!
    int rr = io_uring_register_files(&ring, pipes, num_pipes * 2);

    printf("rr: %d\n", rr);

    //#define BUF_SIZE BUF_SIZE


    for (int i = 0; i < 64; i++) {
        buffers[i].iov_base = malloc(BUF_SIZE);
        buffers[i].iov_len = BUF_SIZE;

        memset(buffers[i].iov_base, 'R', BUF_SIZE);
        if (i % 2) {
            memset(buffers[i].iov_base, 'W', BUF_SIZE);
        }
       
    }
    //buffers[0].

    int e = io_uring_register_buffers(&ring, buffers, 64);

    printf("io_uring_register_buffers: %d\n", e);

    clock_t start = clock();

    /* Start the most minimal NOP-loop possible */
    io_uring_minimal_nop_loop(&ring);

    clock_t end = clock();

    double cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;

    printf("Time: %f\n", cpu_time_used);

        ((char *) buffers[0].iov_base)[6] = 0;
    printf("%s\n", buffers[0].iov_base);
}