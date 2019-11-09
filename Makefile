main: main.o  disk.o sfs.o 
	g++ -o main main.o  disk.o sfs.o   `pkg-config fuse --cflags --libs`
main.o: main.c disk.h sfs.h
	g++ -c -g main.c `pkg-config fuse --cflags --libs`
fuse.o: fuse.c fuse.h
	gcc -c -g fuse.c  `pkg-config fuse --cflags --libs`
sfs.o: sfs.c sfs.h
	g++ -c -g sfs.c `pkg-config fuse --cflags --libs`
disk.o: disk.c disk.h
	g++ -c -g disk.c  `pkg-config fuse --cflags --libs`