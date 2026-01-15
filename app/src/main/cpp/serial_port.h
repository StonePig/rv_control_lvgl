// Minimal serial port helper for Android NDK (termios)
#ifndef SERIAL_PORT_H
#define SERIAL_PORT_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*serial_data_cb_t)(const uint8_t *data, size_t len);

// Open serial device, returns fd or -1 on error
int serial_open(const char *device, int baudrate);

// Read/write wrappers
size_t serial_read(int fd, void *buf, size_t len);
size_t serial_write(int fd, const void *buf, size_t len);

// Close serial device
void serial_close(int fd);

// Start/stop internal reader thread which will call cb on data (runs detached)
// Returns 0 on success
int serial_start_reader(int fd, serial_data_cb_t cb);
int serial_stop_reader(void);

#ifdef __cplusplus
}
#endif

#endif // SERIAL_PORT_H
