// SPDX-License-Identifier: MIT
/*
 * Copyright © 2013 Red Hat, Inc.
 */

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/uinput.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "../kselftest_harness.h"

#define TEST_DEVICE_NAME "selftest input device"

struct selftest_uinput {
        int uinput_fd; /** file descriptor to uinput */
		int evdev_fd; /** file descriptor to evdev */
        char *name; /** device name */
        char *syspath; /** /sys path */
        char *devnode; /** device node */
};

static int is_event_device(const struct dirent *dent) {
        return strncmp("event", dent->d_name, 5) == 0;
}

static char *
fetch_device_node(const char *path)
{
        struct dirent **namelist;
        char *devnode = NULL;
        int ndev, i;

        ndev = scandir(path, &namelist, is_event_device, alphasort);
        if (ndev <= 0)
                return NULL;

        /* ndev should only ever be 1 */

        for (i = 0; i < ndev; i++) {
                if (!devnode && asprintf(&devnode, "/dev/input/%s", namelist[i]->d_name) == -1)
                        devnode = NULL;
                free(namelist[i]);
        }

        free(namelist);

        return devnode;
}

static int is_input_device(const struct dirent *dent) {
        return strncmp("input", dent->d_name, 5) == 0;
}

static int
fetch_syspath_and_devnode(struct selftest_uinput *uidev) {
#define SYS_INPUT_DIR "/sys/devices/virtual/input/"
        struct dirent **namelist;
        int ndev, i;
        int rc;
        char buf[sizeof(SYS_INPUT_DIR) + 64] = SYS_INPUT_DIR;

        rc = ioctl(uidev->uinput_fd,
                   UI_GET_SYSNAME(sizeof(buf) - strlen(SYS_INPUT_DIR)),
                   &buf[strlen(SYS_INPUT_DIR)]);
        if (rc != -1) {
                uidev->syspath = strdup(buf);
                uidev->devnode = fetch_device_node(buf);
                return 0;
        }

        ndev = scandir(SYS_INPUT_DIR, &namelist, is_input_device, alphasort);
        if (ndev <= 0)
                return -1;

        for (i = 0; i < ndev; i++) {
                int fd, len;

                rc = snprintf(buf, sizeof(buf), "%s%s/name",
                              SYS_INPUT_DIR,
                              namelist[i]->d_name);
                if (rc < 0 || (size_t)rc >= sizeof(buf)) {
                        continue;
                }

                /* created within time frame */
                fd = open(buf, O_RDONLY);
                if (fd < 0)
                        continue;

                len = read(fd, buf, sizeof(buf));
                close(fd);
                if (len <= 0)
                        continue;

                buf[len - 1] = '\0'; /* file contains \n */
                if (strcmp(buf, uidev->name) == 0) {
                        if (uidev->syspath) {
                                /* FIXME: could descend into bit comparison here */
                                fprintf(stderr, "multiple identical devices found. syspath is unreliable\n");
                                break;
                        }

                        rc = snprintf(buf, sizeof(buf), "%s%s",
                                      SYS_INPUT_DIR,
                                      namelist[i]->d_name);

                        if (rc < 0 || (size_t)rc >= sizeof(buf)) {
                                fprintf(stderr, "Invalid syspath, syspath is unreliable\n");
                                break;
                        }

                        uidev->syspath = strdup(buf);
                        uidev->devnode = fetch_device_node(buf);
                }
        }

        for (i = 0; i < ndev; i++)
                free(namelist[i]);
        free(namelist);

        return uidev->devnode ? 0 : -1;
#undef SYS_INPUT_DIR
}

void selftest_uinput_destroy(struct selftest_uinput *uidev)
{
	if (!uidev)
		return;

	if (uidev->uinput_fd >= 0) {
		ioctl(uidev->uinput_fd, UI_DEV_DESTROY, NULL);
	}

	close(uidev->evdev_fd);
	close(uidev->uinput_fd);

	free(uidev->syspath);
	free(uidev->devnode);
	free(uidev->name);
	free(uidev);
}

int
selftest_uinput_create_device(struct selftest_uinput** uidev, ...)
{
	struct selftest_uinput *new_device;
	struct uinput_setup setup;
        va_list args;
	int rc, fd;
        int type;

	new_device = calloc(1, sizeof(struct selftest_uinput));
	if (!new_device)
		return -ENOMEM;

	memset(&setup, 0, sizeof(setup));
	strncpy(setup.name, TEST_DEVICE_NAME, UINPUT_MAX_NAME_SIZE - 1);
	setup.id.vendor = 0x1234; /* sample vendor */
	setup.id.product = 0x5678; /* sample product */
	setup.id.bustype = BUS_USB;

	fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
	if (fd < 0) {
		fprintf(stderr, "cannot open uinput (%d): %m\n", errno);
                goto error;
	}

        va_start(args, uidev);
        rc = 0;
        do {
                type = va_arg(args, int);
                if (type == -1)
                        break;
                rc = ioctl(fd, UI_SET_EVBIT, type);
        } while (rc == 0);
        va_end(args);

	rc = ioctl(fd, UI_DEV_SETUP, &setup);
	if (rc == -1)
		goto error;

	rc = ioctl(fd, UI_DEV_CREATE, NULL);
	if (rc == -1)
		goto error;

	new_device->name = strdup(TEST_DEVICE_NAME);
	new_device->uinput_fd = fd;

	if (fetch_syspath_and_devnode(new_device) == -1) {
		fprintf(stderr, "unable to fetch syspath or device node.\n");
		errno = ENODEV;
		goto error;
	}

	fd = open(new_device->devnode, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "cannot open uinput (%d): %m\n", errno);
		goto error;
	}
	new_device->evdev_fd = fd;

	*uidev = new_device;

	return 0;

error:
	rc = -errno;
	selftest_uinput_destroy(new_device);
	return rc;
}

const char* selftest_uinput_get_devnode(struct selftest_uinput *uidev)
{
	return uidev->devnode;
}

TEST(eviocgname_get_device_name)
{
        struct selftest_uinput *uidev;
        char buf[256];
        int rc;

        rc = selftest_uinput_create_device(&uidev);
        ASSERT_EQ(0, rc);
        ASSERT_NE(NULL, uidev);

	memset(buf, 0, sizeof(buf));
        /* ioctl to get the name */
        rc = ioctl(uidev->evdev_fd, EVIOCGNAME(sizeof(buf) - 1), buf);
	ASSERT_GE(rc, 0);
	ASSERT_STREQ(TEST_DEVICE_NAME, buf);

        selftest_uinput_destroy(uidev);
}

TEST(eviocgrep_get_repeat_settings)
{
        struct selftest_uinput *uidev;
        int rep_values[2];
        int rc;

	memset(rep_values, 0, sizeof(rep_values));

        rc = selftest_uinput_create_device(&uidev);
        ASSERT_EQ(0, rc);
        ASSERT_NE(NULL, uidev);

        /* ioctl to get the repeat rates values */
        rc = ioctl(uidev->evdev_fd, EVIOCSREP, rep_values);
        /* should fail because EV_REP is not set */
	ASSERT_EQ(-1, rc);

        selftest_uinput_destroy(uidev);

        rc = selftest_uinput_create_device(&uidev, EV_REP);
        ASSERT_EQ(0, rc);
        ASSERT_NE(NULL, uidev);

        /* ioctl to get the repeat rates values */
        rc = ioctl(uidev->evdev_fd, EVIOCGREP, rep_values);
        ASSERT_EQ(0, rc);
        /* should get the default delay an period values set by the kernel */
        ASSERT_EQ(rep_values[0], 250);
        ASSERT_EQ(rep_values[1], 33);

       selftest_uinput_destroy(uidev);
}

TEST_HARNESS_MAIN
