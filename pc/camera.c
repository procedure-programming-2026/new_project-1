
#include "camera.h"
#include <sys/mman.h>
#include <stdio.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <stdlib.h>

struct buffer
{
    void *start;
    size_t length;
};

static struct buffer *buffers = NULL;
static unsigned int buffer_count = 0;

int camera_info(int fd)
{
    struct v4l2_capability caps = {0};

    int result = ioctl(fd, VIDIOC_QUERYCAP, &caps);

    if (result == -1)
    {
        perror("Camera information error");
        return 1;
    }

    printf("Driver: %s\n", caps.driver);
    printf("Camera: %s\n", caps.card);
    printf("Bus: %s\n", caps.bus_info);

    return 0;
}

int camera_format(int fd)
{
    struct v4l2_format fmt = {0};

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = 640;
    fmt.fmt.pix.height = 480;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    fmt.fmt.pix.field = V4L2_FIELD_NONE;

    int result = ioctl(fd, VIDIOC_S_FMT, &fmt);

    if (result == -1)
    {
        perror("Camera format error");
        return 1;
    }

    printf("Width: %u\n", fmt.fmt.pix.width);
    printf("Height: %u\n", fmt.fmt.pix.height);

    if (fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_MJPEG)
    {
        printf("MJPEG format is not supported\n");
        return 1;
    }

    printf("Camera format configured successfully!\n");

    return 0;
}

void camera_cleanup(void)
{
    for (unsigned int i = 0; i < buffer_count; i++)
    {
        if (buffers[i].start != NULL &&
            buffers[i].start != MAP_FAILED)
        {
            if (munmap(buffers[i].start, buffers[i].length) == -1)
            {
                perror("Memory unmapping error");
            }
        }
    }

    free(buffers);
    buffers = NULL;
    buffer_count = 0;
}

int camera_buffers(int fd)
{
    struct v4l2_requestbuffers req = {0};

    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    int result = ioctl(fd, VIDIOC_REQBUFS, &req);

    if (result == -1)
    {
        perror("Buffer allocation error");
        return 1;
    }

    if (req.count < 2)
    {
        printf("Not enough buffers\n");
        return 1;
    }

    printf("Allocated buffers: %u\n", req.count);

    buffers = calloc(req.count, sizeof(struct buffer));

    if (buffers == NULL)
    {
        perror("Memory allocation error");
        return 1;
    }

    buffer_count = req.count;

    for (unsigned int i = 0; i < buffer_count; i++)
    {
        struct v4l2_buffer buf = {0};

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        result = ioctl(fd, VIDIOC_QUERYBUF, &buf);

        if (result == -1)
        {
            perror("Buffer query error");
            camera_cleanup();
            return 1;
        }

        buffers[i].length = buf.length;

        buffers[i].start = mmap(
            NULL,
            buf.length,
            PROT_READ | PROT_WRITE,
            MAP_SHARED,
            fd,
            buf.m.offset
        );

        if (buffers[i].start == MAP_FAILED)
        {
            perror("Memory mapping error");
            camera_cleanup();
            return 1;
        }
    }

    return 0;
}

int camera_queue(int fd)
{
    for (unsigned int i = 0; i < buffer_count; i++)
    {
        struct v4l2_buffer buf = {0};

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        int result = ioctl(fd, VIDIOC_QBUF, &buf);

        if (result == -1)
        {
            perror("Buffer queue error");
            return 1;
        }
    }

    return 0;
}

int camera_start(int fd)
{
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    int result = ioctl(fd, VIDIOC_STREAMON, &type);

    if (result == -1)
    {
        perror("Stream start error");
        return 1;
    }

    printf("Camera streaming started!\n");

    return 0;
}

int camera_stop(int fd)
{
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    int result = ioctl(fd, VIDIOC_STREAMOFF, &type);

    if (result == -1)
    {
        perror("Stream stop error");
        return 1;
    }

    printf("Camera streaming stopped!\n");

    return 0;
}

int camera_capture(int fd)
{
    struct v4l2_buffer buf = {0};

    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    int result = ioctl(fd, VIDIOC_DQBUF, &buf);

    if (result == -1)
    {
        perror("Frame capture error");
        return 1;
    }

    if (buf.index >= buffer_count ||
        buf.bytesused > buffers[buf.index].length)
    {
        fprintf(stderr, "Invalid frame buffer\n");
        return 1;
    }

    printf("Frame index: %u\n", buf.index);
    printf("Frame size: %u bytes\n", buf.bytesused);

    int save_result = 0;

    FILE *file = fopen("../images/frame.jpg", "wb");

    if (file == NULL)
    {
        perror("File opening error");
        save_result = 1;
    }
    else
    {
        size_t written = fwrite(
            buffers[buf.index].start,
            1,
            buf.bytesused,
            file
        );

        if (written != buf.bytesused)
        {
            fprintf(stderr, "Frame writing error\n");
            save_result = 1;
        }

        if (fclose(file) != 0)
        {
            perror("File closing error");
            save_result = 1;
        }
    }

    result = ioctl(fd, VIDIOC_QBUF, &buf);

    if (result == -1)
    {
        perror("Buffer requeue error");
        return 1;
    }

    if (save_result == 0)
    {
        printf("Frame saved successfully!\n");
    }

    return save_result;
}