/*
 * Keyboard input via /dev/input/eventX
 *
 * Uses non-blocking I/O to read key press events from a Linux
 * input device. Maps arrow keys, space, D, and ESC to game actions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include "input.h"

static int input_fd = -1;

int input_init(const char *device_path)
{
    input_fd = open(device_path, O_RDONLY | O_NONBLOCK);
    if (input_fd < 0) {
        perror("input_init: open");
        return -1;
    }
    return 0;
}

int input_poll(void)
{
    struct input_event ev;

    if (input_fd < 0)
        return INPUT_NONE;

    while (read(input_fd, &ev, sizeof(ev)) == sizeof(ev)) {
        /* Only handle key press events (value == 1) */
        if (ev.type != EV_KEY || ev.value != 1)
            continue;

        switch (ev.code) {
        case KEY_UP:    return INPUT_UP;
        case KEY_DOWN:  return INPUT_DOWN;
        case KEY_LEFT:  return INPUT_LEFT;
        case KEY_RIGHT: return INPUT_RIGHT;
        case KEY_SPACE: return INPUT_SPACE;
        case KEY_D:     return INPUT_D;
        case KEY_ESC:   return INPUT_ESC;
        case KEY_Q:     return INPUT_Q;
        case KEY_W:     return INPUT_W;
        default:        break;
        }
    }

    return INPUT_NONE;
}

void input_close(void)
{
    if (input_fd >= 0) {
        close(input_fd);
        input_fd = -1;
    }
}
