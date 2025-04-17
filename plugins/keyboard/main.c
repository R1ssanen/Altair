#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "altair.h"

ALAPI const u32   type = PLUGIN_KEYBOARD | PLUGIN_ASYNC;

AL_PluginManager* manager;
struct pollfd     fd;

ALAPI b8          init(AL_PluginManager* manager_) {
    manager   = manager_;

    fd.fd     = open("/dev/input/event4\0", O_RDONLY | O_NONBLOCK);
    fd.events = POLLIN;

    if (fd.fd == -1) {
        perror("Er"); // LERROR("%s", stderror(errno));
        return false;
    }

    return true;
}

typedef struct {
    struct timeval time;
    __U16_TYPE     type;
    __U16_TYPE     code;
    __S32_TYPE     value;
} InputEvent;

u64       event_size = sizeof(InputEvent);
u8        max_events = 10;

ALAPI u32 proc(AL_Plugin* self) {
    LINFO("Proc enter.");
    u8 buffer[event_size * max_events];

    if (poll(&fd, 1, 100) <= 0) {
        LINFO("Timeout");
        return 1;
    }

    if (!fd.revents) {
        LINFO("Revent");
        return 1;
    }

    ssize_t bytes_read = read(fd.fd, (void*)buffer, event_size * max_events);
    if (bytes_read == -1) {
        switch (errno) {
        case EAGAIN: LINFO("No activity"); return 1;
        }
    }

    // LINFO("Total bytes read: %llu / %llu", bytes_read, event_size * max_events);

    for (u64 byte = 0; byte < bytes_read; byte += event_size) {
        InputEvent* event = (InputEvent*)(buffer + byte);
        if (event->type != EV_KEY) continue;

        switch (event->code) {
        case KEY_Q:
            if (event->value) LINFO("Q pressed.");
            else
                LINFO("Q released.");
            break;

        default: break;
        }
    }

    return 1;
}

ALAPI b8 cleanup(void) {
    close(fd.fd);
    return true;
}
