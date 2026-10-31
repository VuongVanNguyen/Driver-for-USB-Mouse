// SPDX-License-Identifier: GPL-2.0
/*
 * event_reader.c - Đọc sự kiện từ /dev/input/eventX và in ra dạng dễ đọc.
 * Dùng: sudo ./event_reader /dev/input/eventX
 */
#include <fcntl.h>
#include <linux/input.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

static const char *key_name(unsigned int code)
{
	switch (code) {
	case BTN_LEFT:		return "LEFT";
	case BTN_RIGHT:		return "RIGHT";
	case BTN_MIDDLE:	return "MIDDLE";
	case BTN_SIDE:		return "SIDE";
	case BTN_EXTRA:		return "EXTRA";
	default:		return "?";
	}
}

static const char *rel_name(unsigned int code)
{
	switch (code) {
	case REL_X:		return "X";
	case REL_Y:		return "Y";
	case REL_WHEEL:		return "WHEEL";
	default:		return "?";
	}
}

int main(int argc, char **argv)
{
	struct input_event ev;
	char name[256] = "?";
	int fd;

	if (argc != 2) {
		fprintf(stderr, "Dùng: %s /dev/input/eventX\n", argv[0]);
		return 1;
	}

	fd = open(argv[1], O_RDONLY);
	if (fd < 0) {
		perror(argv[1]);
		return 1;
	}

	ioctl(fd, EVIOCGNAME(sizeof(name)), name);
	printf("Device: %s\n", name);

	while (read(fd, &ev, sizeof(ev)) == sizeof(ev)) {
		switch (ev.type) {
		case EV_KEY:
			printf("KEY %-6s %s\n", key_name(ev.code), ev.value ? "down" : "up");
			break;
		case EV_REL:
			printf("REL %-6s %+d\n", rel_name(ev.code), ev.value);
			break;
		case EV_SYN:
			printf("--- SYN\n");
			break;
		}
		fflush(stdout);
	}

	close(fd);
	return 0;
}
