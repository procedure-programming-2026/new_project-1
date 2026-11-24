
CC = gcc

CFLAGS = -Wall -Wextra -O2 -I../include

SDL_CFLAGS = $(shell sdl2-config --cflags)
SDL_LIBS = $(shell sdl2-config --libs)

STREAM_SRC = stream_test.c \
             stream.c \
             image_processing.c \
             marker_tracking.c \
             motion_detection.c \
             skin_detection.c \
             hand_detection.c \
             visual_debugger.c

all: camera stream_test

camera: main.c camera.c
	$(CC) $(CFLAGS) main.c camera.c -o camera

stream_test: $(STREAM_SRC)
	$(CC) $(CFLAGS) $(SDL_CFLAGS) $(STREAM_SRC) -o stream_test $(SDL_LIBS) -lm

clean:
	rm -f camera stream_test sdl_test

.PHONY: all clean
