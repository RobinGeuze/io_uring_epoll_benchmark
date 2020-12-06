default:
	gcc -O3 main.c -o main
	gcc -O3 io.c /usr/lib/liburing.a -o io
