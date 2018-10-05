CC=gcc
#LDFLAGS=-L /usr/local/lib -Wl,-rpath /usr/local/lib
LIBS=-lmicrohttpd -ljansson -lwebsockets -pthread

jsmpegserver: main.o httpd.o websocketd.o
	$(CC) -o $@ $^ $(LIBS)

main.o: main.c httpd.h websocketd.h
	$(CC) -c -o $@ main.c

httpd.o: httpd.c websocketd.h
	$(CC) -c -o $@ httpd.c

websocketd.o: websocketd.c main.h websocketd.h
	$(CC) -c -o $@ websocketd.c

clean:
	rm *.o