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

struct pollfd     poller;
AL_PluginManager* manager;

ALAPI b8          init(AL_PluginManager* manager_) {
    manager       = manager_;
    poller.fd     = 0; // open("/dev/input/event4", O_RDONLY | O_NONBLOCK);
    poller.events = POLLIN;

    if (poller.fd == -1) {
        switch (errno) {
        case EACCES: LERROR("No permission to read /dev/input/."); break;
        default: LERROR("Could not initialize keyboard."); break;
        }
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
    u8         buffer[event_size * max_events];
    AL_Thread* thread = &self->opt.thread;

    AL_AsyncWhile(&thread->mutex, SYNC_EXIT) {
        if (poll(&poller, 1, 100) <= 0) continue;
        if (!poller.revents) continue;

        ssize_t bytes_read = 0; // read(poller.fd, (void*)buffer, event_size * max_events);
        if (bytes_read == -1) {
            switch (errno) {
            case EAGAIN: break;
            default: break;
            }
            continue;
        }

        for (u64 byte = 0; byte < bytes_read; byte += event_size) {
            InputEvent* event = (InputEvent*)(buffer + byte);
            if (event->type != EV_KEY) continue;

            switch (event->code) {
            case KEY_Q:
                AL_WriteSyncFlag(&manager->mutex, SYNC_EXIT);
                // goto end;
                break;
            default: break;
            }
        }
    }

end:
    LNOTE("Exiting");
    AL_WakeCondition(&thread->mutex);
    return true;
}

ALAPI b8 cleanup(void) {
    close(poller.fd);
    return true;
}
