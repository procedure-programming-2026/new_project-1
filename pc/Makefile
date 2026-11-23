
CC = gcc
CFLAGS = -Wall -Wextra -I../include

all: camera stream_test

camera: main.c camera.c
	$(CC) $(CFLAGS) main.c camera.c -o camera

stream_test: stream_test.c stream.c image_processing.c marker_tracking.c
	$(CC) $(CFLAGS) stream_test.c stream.c image_processing.c marker_tracking.c -o stream_test -lm

clean:
	rm -f camera stream_test

.PHONY: all clean
