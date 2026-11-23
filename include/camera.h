#ifndef CAMERA_H
#define CAMERA_H

int camera_info(int fd);
int camera_format(int fd);
void camera_cleanup(void);
int camera_buffers(int fd);
int camera_queue(int fd);
int camera_start(int fd);
int camera_stop(int fd);
int camera_capture(int fd);

#endif