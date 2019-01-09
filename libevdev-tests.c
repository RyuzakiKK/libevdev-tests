/* libevdev-tests.c
 *
 * Copyright 2019 RyuzakiKK
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE X CONSORTIUM BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name(s) of the above copyright
 * holders shall not be used in advertising or otherwise to promote the sale,
 * use or other dealings in this Software without prior written
 * authorization.
 */

/* This is a simple script that grab a device (keyboard) and print the pressed
 * keys.
 * WARNING: As long as this script runs the grab is kept. So I suggest you to
 * use a secondary keyboard or you could lock yourself out. */

/* mouse-dpi-tool.c has bee used as a base for this test script
 * https://gitlab.freedesktop.org/libevdev/libevdev/blob/master/tools/mouse-dpi-tool.c */

#include <libevdev/libevdev.h>
#include <sys/signalfd.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static int
usage(void) {
	printf("Usage: libevdev-tests /dev/input/eventXX\n");
  printf("Probably you need to execute it with root access.\n");
	return 1;
}

static int
handle_event(const struct input_event *ev)
{
  /* We print the pressed key if the event is an EV_KEY.
   * If ev->value is 0 it means that we received a key released event. */
  if (ev->type == EV_KEY && ev->value == 1)
    printf("code %hu, (%s)\n",
           ev->code,
           libevdev_event_code_get_name(ev->type, ev->code));

	return 0;
}

static int
mainloop(struct libevdev *dev) {
	struct pollfd fds[2];
	sigset_t mask;

	fds[0].fd = libevdev_get_fd(dev);
	fds[0].events = POLLIN;

	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	fds[1].fd = signalfd(-1, &mask, SFD_NONBLOCK);
	fds[1].events = POLLIN;

	sigprocmask(SIG_BLOCK, &mask, NULL);

	while (poll(fds, 2, -1)) {
		struct input_event ev;
		int rc;

		if (fds[1].revents)
			break;

		do {
			rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);
			if (rc == LIBEVDEV_READ_STATUS_SYNC) {
				fprintf(stderr, "Error: cannot keep up\n");
				return 1;
			} else if (rc != -EAGAIN && rc < 0) {
				fprintf(stderr, "Error: %s\n", strerror(-rc));
				return 1;
			} else if (rc == LIBEVDEV_READ_STATUS_SUCCESS) {
				handle_event(&ev);
			}
		} while (rc != -EAGAIN);
	}

	return 0;
}

int
main (int argc, char **argv) {
	int rc;
	int fd;
	const char *path;
	struct libevdev *dev;

	if (argc < 2)
		return usage();

	path = argv[1];
	if (path[0] == '-')
		return usage();

	fd = open(path, O_RDONLY|O_NONBLOCK);
	if (fd < 0) {
		fprintf(stderr, "Error opening the device: %s\n", strerror(errno));
		return 1;
	}

	rc = libevdev_new_from_fd(fd, &dev);
	if (rc != 0) {
		fprintf(stderr, "Error fetching the device info: %s\n", strerror(-rc));
		return 1;
	}

	if (libevdev_grab(dev, LIBEVDEV_GRAB) != 0) {
		fprintf(stderr, "Error: cannot grab the device, something else is grabbing it.\n");
		fprintf(stderr, "Use 'fuser -v %s' to find processes with an open fd\n", path);
		return 1;
	}

  /* We are keeping the grab, so we will be the only one receiving the device events. */
	rc = mainloop(dev);

  libevdev_grab(dev, LIBEVDEV_UNGRAB);
	libevdev_free(dev);
	close(fd);

	return rc;
}

