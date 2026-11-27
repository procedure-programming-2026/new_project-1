#define _DEFAULT_SOURCE
#include "serial_comm.h"
#include <stdio.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>

static unsigned char calc_xor_checksum(const char *payload){
    unsigned char cs = 0;
    for (int i=0; payload[i]!='\0'; i++){
        cs ^= (unsigned char)payload[i];
    }
    return cs;
}

static int open_port(const char *port_name){
    int fd= open(port_name, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0){
        return -1;
    }
    int bits = TIOCM_DTR | TIOCM_RTS;
    ioctl(fd, TIOCMBIC, &bits);
    struct termios tty;
    memset(&tty, 0, sizeof(tty));
    if (tcgetattr(fd, &tty) != 0){
        close(fd);
        return -1;
    }
    cfsetospeed(&tty, B115200);
    cfsetispeed(&tty, B115200);

    tty.c_cflag =(tty.c_cflag & ~CSIZE) | CS8;
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~(PARENB | PARODD | CSTOPB | CRTSCTS | HUPCL);
    tty.c_iflag = 0;
    tty.c_oflag = 0;
    tty.c_lflag = 0;
    tcflush(fd, TCIFLUSH);
    if (tcsetattr(fd, TCSANOW, &tty) != 0){
        close(fd);
        return -1;
    }
    ioctl(fd, TIOCMBIC, &bits);
    return fd;
}

int serial_init(SerialPort *sp, const char *port_name){
    memset(sp, 0, sizeof(SerialPort));
    snprintf(sp->port_name, sizeof(sp->port_name), "%s",port_name);
    sp->fd = open_port(port_name);
    sp->connected = (sp->fd >= 0);

    if (sp->connected){
        usleep(1500000);
        printf("[SERIAL] CONNECTED TO %s\n", sp->port_name);
    }else{
        printf("[SERIAL] DISCONNECTED (%s not ready, will retry...)", sp->port_name);
    }
    return sp->connected;
}

void serial_send_state(SerialPort *sp, Direction dir, const Marker *marker){
    if (!sp->connected) {
        open_port(sp->port_name);
        if (sp->fd >= 0){
            sp->connected = 1;
            printf("[SERIAL] RECONNECT to %s", sp->port_name);
        }else{
            return;
        }
    }
    const char *dir_str = direction_to_string(dir);
    int x = marker->found ? marker->x : -1;
    int y = marker->found ? marker->y : -1;

    char payload[64];
    snprintf(payload, sizeof(payload), "%s,%d,%d,%d,%d", dir_str, x, y, marker->found, marker->grip);

    unsigned char cs = calc_xor_checksum(payload);

    char packet[80];
    int len = snprintf(packet, sizeof(packet), "$%s*%02X\n", payload, cs);

    ssize_t written = write(sp->fd, packet, len);
    if (dir != DIRECTION_STOP) printf("[TX] %s", packet);
    if (written < 0 && errno != EAGAIN && errno != EWOULDBLOCK){
        printf("[SERIAL] CONNECTION LOST! Switching to reconnect mode...\n");
        close(sp->fd);
        sp->fd = -1;
        sp->connected = 0;
    }
    // char rx_buf[256];
    // ssize_t n = read(sp->fd, rx_buf, sizeof(rx_buf)-1);
    // if (n >0){
    //     rx_buf[n] = '\0';
    //     printf("%s", rx_buf);
    // }
}

void serial_close(SerialPort *sp){
    if (sp->fd >= 0){
        close(sp->fd);
        sp->fd = -1;
    }
    sp->connected = 0;
}