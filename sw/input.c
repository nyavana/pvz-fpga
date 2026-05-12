/*
 * Keyboard / gamepad input via /dev/input/eventX
 *
 * Uses non-blocking I/O to read events from a Linux input device.
 * Maps both keyboard keys and Xbox 360-compatible gamepad buttons
 * (xpad: BTN_DPAD_*, BTN_SOUTH/EAST, BTN_TL/TR, BTN_START, plus
 * ABS_HAT0X/Y for variants that report the D-pad as hat axes) to
 * the same INPUT_* action codes consumed by main.c.
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
        /* Some xpad variants report the D-pad as absolute hat axes
         * rather than BTN_DPAD_* keys. value == 0 is the release
         * event; ignore it so an idle hat doesn't spam INPUT_NONE. */
        if (ev.type == EV_ABS) {
            if (ev.code == ABS_HAT0X) {
                if (ev.value < 0) return INPUT_LEFT;
                if (ev.value > 0) return INPUT_RIGHT;
            } else if (ev.code == ABS_HAT0Y) {
                if (ev.value < 0) return INPUT_UP;
                if (ev.value > 0) return INPUT_DOWN;
            }
            continue;
        }

        /* Only handle key/button press events (value == 1) */
        if (ev.type != EV_KEY || ev.value != 1)
            continue;

        switch (ev.code) {
        case KEY_UP:    case BTN_DPAD_UP:    return INPUT_UP;
        case KEY_DOWN:  case BTN_DPAD_DOWN:  return INPUT_DOWN;
        case KEY_LEFT:  case BTN_DPAD_LEFT:  return INPUT_LEFT;
        case KEY_RIGHT: case BTN_DPAD_RIGHT: return INPUT_RIGHT;
        case KEY_SPACE: case BTN_SOUTH:      return INPUT_SPACE;
        case KEY_D:     case BTN_EAST:       return INPUT_D;
        case KEY_ESC:   case BTN_START:      return INPUT_ESC;
        case KEY_TAB:   case BTN_TL: case BTN_TR: return INPUT_TAB;
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
