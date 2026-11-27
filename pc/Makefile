CC = gcc

CFLAGS = -Wall -Wextra -O2 -I../include

SDL_CFLAGS = $(shell sdl2-config --cflags)
SDL_LIBS = $(shell sdl2-config --libs)
SDL_TTF_LIBS = -lSDL2_ttf

STREAM_SRC = stream_test.c \
             stream.c \
             image_processing.c \
             marker_tracking.c \
             motion_detection.c \
             skin_detection.c \
             hand_detection.c \
             visual_debugger.c \
             serial_comm.c 


PHOTORUKA_SRC = photoruka.c \
                camera_thread.c \
                gui.c \
                stream.c \
                image_processing.c \
                marker_tracking.c \
                motion_detection.c \
                skin_detection.c \
                hand_detection.c

all: camera stream_test photoruka

camera: main.c camera.c
	$(CC) $(CFLAGS) main.c camera.c -o camera

stream_test: $(STREAM_SRC)
	$(CC) $(CFLAGS) $(SDL_CFLAGS) $(STREAM_SRC) -o stream_test $(SDL_LIBS) -lm

photoruka: $(PHOTORUKA_SRC)
	$(CC) $(CFLAGS) $(SDL_CFLAGS) $(PHOTORUKA_SRC) -o photoruka $(SDL_LIBS) $(SDL_TTF_LIBS) -lpthread -lm

clean:
	rm -f camera stream_test photoruka sdl_test

.PHONY: all clean
