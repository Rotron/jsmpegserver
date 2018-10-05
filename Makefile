CC=g++
LIBS=-lmicrohttpd -ljansson -lwebsockets

jsmpegserver: main.o httpd.o websocketd.o
	gcc -o $@ $^ $(LIBS)

main.o: main.c httpd.h websocketd.h
	gcc -c -o $@ main.c

httpd.o: httpd.c websocketd.h
	gcc -c -o $@ httpd.c

websocketd.o: websocketd.c main.h websocketd.h
	gcc -c -o $@ websocketd.c

clean:
	rm *.o