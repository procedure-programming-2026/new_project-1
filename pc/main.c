
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include "camera.h"

static const char CAMERA[] = "/dev/video0";

int main(void)
{
    int fd = open(CAMERA, O_RDWR);

    if (fd < 0)
    {
        perror("Camera opening error");
        return 1;
    }

    printf("Camera opened successfully!\n");

    int result = camera_info(fd);

    if (result == 0)
    {
        result = camera_format(fd);
    }

    if (result == 0)
    {
        result = camera_buffers(fd);
    }

    if (result == 0)
    {
        result = camera_queue(fd);
    }

    if (result == 0)
    {
        result = camera_start(fd);

        if (result == 0)
        {
            result = camera_capture(fd);

            int stop_result = camera_stop(fd);

            if (result == 0)
            {
                result = stop_result;
            }
        }
    }

    camera_cleanup();
    close(fd);

    return result;
}
