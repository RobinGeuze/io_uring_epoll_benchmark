default:
	gcc -O3 epoll.c -o epoll
	gcc -O3 io_uring.c /usr/lib/liburing.a -o io_uring

runs:
	rm -f epoll_runs
	rm -f io_uring_runs
	for i in {100..9000..100}; do ./io_uring $$i; done
	for i in {100..9000..100}; do ./epoll $$i; done