default:
	gcc -O3 epoll.c -o epoll
	gcc -O3 io_uring.c /usr/lib/liburing.a -o io_uring
