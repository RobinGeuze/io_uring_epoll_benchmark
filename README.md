# IO_uring, epoll benchmark
This is a scientific test, measuring the performance of epoll/io_uring when writing/reading small messages.
Even though theory clearly points in one direction (such as io_uring **obviously** being faster than epoll because of eliminated syscalls and copies), practical benchmarks can be important reality checks.

In order to eliminate as much noise as possible, pipes are used instead of sockets. By using pipes, we can eliminate the entire networking subsystem and all the variability it brings.

Similar tests have been run with actual TCP socktes earlier, and with similar conclusion, but this test is run with pipes as a more minimal and direct appraoch for comparison.

## The test
A varying amount of non-blocking pipes are used in writing/reading/polling messages of 1KB. The numbers are for currently active pipes, not concurrent-but-idle pipes.
As such, the maximum load plotted should be comparable to multiple thousands of concurrent connections.

<img src="Time%20elapsed%20for%2010k%20write_read%20of%201KB%20data%20(lower%20is%20better%2C%20Linux%205.9.9).png" />

## About io_uring
IO_uring is a way of communicating with the kernel without doing any syscalls or copies. The above test is run with io_uring in this mode, where no syscalls are being made.
Buffers are pre-registered and fixed and file descriptors are pre-registered - all in the name of advanced and performant io_uring use. Strace shows zero syscalls being made while testing.
With epoll, millions of syscalls are being made and copies are performed from/to kernel space. Nothing is pre-registered here.


## Conclusion
The immediate theory is that the fewer the syscalls and copies, the better the performance. This might be true in theory, but the above graph clearly points towards this not being the case here.
I reserve the possibility of having done something wrong with my use of io_uring, but given that I have tested about 5 different projects made by 5 different authors by now, all showing the same general outcome - I'm quite confident in my findings.

I'm sure Facebook can show certain cases where io_uring outpeforms epoll, but in my testing, I have no been able to find one single case where it does so. I have tested on two separate native machines with two entirely different Linux kernels installed, and still got the same general outcome.

Mob mentality and hype always leads you to people **claiming** this or that, but actual real benchmarks that can show this incredible gain of io_uring have still to make it to me. I am waiting.
