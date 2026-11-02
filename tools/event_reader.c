// SPDX-License-Identifier: GPL-2.0

/*
 * event_reader.c
 * Doc su kien tu /dev/input/eventX va hien thi ket qua.
 * Cach dung: sudo ./event_reader /dev/input/eventX
 */

#include <fcntl.h>
#include <linux/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>

static const char *key_name(unsigned int code)
{
    switch (code) {
    case BTN_LEFT:
        return "LEFT";
    case BTN_RIGHT:
        return "RIGHT";
    case BTN_MIDDLE:
        return "MIDDLE";
    case BTN_SIDE:
        return "SIDE";
    case BTN_EXTRA:
        return "EXTRA";
    default:
        return "UNKNOWN";
    }
}

static const char *rel_name(unsigned int code)
{
    switch (code) {
    case REL_X:
        return "X";
    case REL_Y:
        return "Y";
    case REL_WHEEL:
        return "WHEEL";
    default:
        return "UNKNOWN";
    }
}

static const char *key_state(int value)
{
    switch (value) {
    case 0:
        return "UP";
    case 1:
        return "DOWN";
    case 2:
        return "REPEAT";
    default:
        return "UNKNOWN";
    }
}

int main(int argc, char **argv)
{
    struct input_event ev;
    char name[256] = "Unknown device";
    ssize_t bytes_read;
    int fd;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s /dev/input/eventX\n", argv[0]);
        return EXIT_FAILURE;
    }

    fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("Cannot open input device");
        return EXIT_FAILURE;
    }

    if (ioctl(fd, EVIOCGNAME(sizeof(name)), name) < 0) {
        perror("Cannot get device name");
        close(fd);
        return EXIT_FAILURE;
    }

    printf("Device: %s\n", name);
    printf("Listening for mouse events...\n");

    while (1) {
        bytes_read = read(fd, &ev, sizeof(ev));

        if (bytes_read < 0) {
            if (errno == EINTR)
                continue;

            perror("Read error");
            break;
        }

        if (bytes_read == 0)
            break;

        if (bytes_read != sizeof(ev)) {
            fprintf(stderr, "Incomplete event data\n");
            break;
        }

        switch (ev.type) {
        case EV_KEY:
            printf("KEY %-8s %s\n",
                   key_name(ev.code),
                   key_state(ev.value));
            break;

        case EV_REL:
            printf("REL %-8s %+d\n",
                   rel_name(ev.code),
                   ev.value);
            break;

        case EV_SYN:
            if (ev.code == SYN_REPORT)
                printf("--- SYN_REPORT ---\n");
            break;

        default:
            break;
        }

        fflush(stdout);
    }

    close(fd);

    return EXIT_SUCCESS;
}
