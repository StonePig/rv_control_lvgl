#include "serial_port.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <pthread.h>
#include <android/log.h>

#define LOG_TAG "serial_port"
#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static speed_t baud_to_constant(int baud)
{
    switch (baud)
    {
    case 9600:
        return B9600;
    case 19200:
        return B19200;
    case 38400:
        return B38400;
    case 57600:
        return B57600;
    case 115200:
        return B115200;
    case 230400:
        return B230400;
    default:
        return B115200;
    }
}

int serial_open(const char *device, int baudrate)
{
    if (!device)
        return -1;

    int fd = open(device, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0)
    {
        ALOGE("open(%s) failed: %s", device, strerror(errno));
        return -1;
    }

    struct termios tty;
    if (tcgetattr(fd, &tty) != 0)
    {
        ALOGE("tcgetattr failed: %s", strerror(errno));
        close(fd);
        return -1;
    }

    cfmakeraw(&tty);

    // set baud
    speed_t speed = baud_to_constant(baudrate);
    cfsetispeed(&tty, speed);
    cfsetospeed(&tty, speed);

    // 8N1
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;

    // no flow control
    tty.c_cflag &= ~CRTSCTS;

    // enable receiver, ignore modem ctrl lines
    tty.c_cflag |= CREAD | CLOCAL;

    // blocking read: set VMIN/VTIME
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 0; // tenths of a second

    if (tcsetattr(fd, TCSANOW, &tty) != 0)
    {
        ALOGE("tcsetattr failed: %s", strerror(errno));
        close(fd);
        return -1;
    }

    // clear non-blocking flag for normal reads
    int flags = fcntl(fd, F_GETFL, 0);
    flags &= ~O_NONBLOCK;
    fcntl(fd, F_SETFL, flags);

    ALOGI("serial opened %s @ %d", device, baudrate);
    return fd;
}

size_t serial_read(int fd, void *buf, size_t len)
{
    if (fd < 0 || !buf)
        return -1;
    size_t r = read(fd, buf, len);
    if (r < 0)
    {
        ALOGE("serial read error: %s", strerror(errno));
    }
    return r;
}

size_t serial_write(int fd, const void *buf, size_t len)
{
    if (fd < 0 || !buf)
        return -1;
    size_t w = write(fd, buf, len);
    if (w < 0)
    {
        ALOGE("serial write error: %s", strerror(errno));
    }
    return w;
}

void serial_close(int fd)
{
    if (fd >= 0)
    {
        close(fd);
        ALOGI("serial closed fd=%d", fd);
    }
}

// Simple single-reader implementation
static pthread_t reader_thread;
static int reader_fd = -1;
static serial_data_cb_t reader_cb = NULL;
static volatile int reader_running = 0;
static uint16_t reader_buf_size = 0;

static void *reader_thread_fn(void *arg)
{
    (void)arg;
    uint8_t buf[512];
    uint8_t read_buf[2048];
    uint8_t time_out_counter = 0;

    while (reader_running)
    {
        if (reader_fd < 0)
        {
            usleep(100 * 1000);
            continue;
        }
        size_t r = serial_read(reader_fd, buf, sizeof(buf));
        if (r > 0)
        {
            time_out_counter = 0;
            if(reader_buf_size + r > sizeof(read_buf))
            {
                // overflow, reset
                reader_buf_size = 0;
            }
            memcpy(read_buf + reader_buf_size, buf, r);
            reader_buf_size += r;
        }
        else if (r == 0)
        {
            if(reader_buf_size)
            {
                if(time_out_counter >= 1)
                {
                    if (reader_cb)
                    {
                        reader_cb(read_buf, reader_buf_size);
                    }
                    // reset buffer if no data received for a while
                    reader_buf_size = 0;
                }
                time_out_counter++;
            }
            usleep(50 * 1000);
        }
        else
        {
            // error
            usleep(200 * 1000);
        }
    }
    return NULL;
}

int serial_start_reader(int fd, serial_data_cb_t cb)
{
    if (fd < 0 || !cb)
        return -1;
    if (reader_running)
        return -1; // already running

    reader_fd = fd;
    reader_cb = cb;
    reader_running = 1;
    if (pthread_create(&reader_thread, NULL, reader_thread_fn, NULL) != 0)
    {
        ALOGE("failed to create reader thread");
        reader_running = 0;
        return -1;
    }
    pthread_detach(reader_thread);
    ALOGI("reader thread started");
    return 0;
}

int serial_stop_reader(void)
{
    if (!reader_running)
        return 0;
    reader_running = 0;
    // reader_thread is detached; wait briefly
    usleep(200 * 1000);
    reader_fd = -1;
    reader_cb = NULL;
    ALOGI("reader thread stopped");
    return 0;
}
