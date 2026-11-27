#ifndef SERIAL_COMM_H
#define SERIAL_COMM_H
#include "marker_tracking.h"
typedef struct {
    int fd;
    int connected;
    char port_name[64];
} SerialPort;
int serial_init(SerialPort *sp, const char *port_name);
void serial_send_state(SerialPort *sp, Direction dr, const Marker *marker);
void serial_close(SerialPort *sp);
#endif