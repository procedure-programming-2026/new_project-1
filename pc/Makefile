
CC = gcc
CFLAGS = -Wall -Wextra -I../include

all: camera stream_test

camera: main.c camera.c ../include/camera.h
	$(CC) $(CFLAGS) main.c camera.c -o camera

stream_test: stream_test.c stream.c image_processing.c ../include/stream.h ../include/image_processing.h
	$(CC) $(CFLAGS) stream_test.c stream.c image_processing.c -o stream_test

clean:
	rm -f camera stream_test

.PHONY: all clean
