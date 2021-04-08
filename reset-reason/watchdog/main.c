/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/watchdog.h>
#include <linux/ioctl.h>
#include <getopt.h>

#define ALLOC_SIZE 64

static void usage(void)
{
    printf ("\nwatchdog-test [OPTIONS...]\n\n"
            "Test if the watchdog will trigger and if reset reason changes\n\n"
            "  -d Watchdog device (e.g. /dev/watchdog)\n"
            "  -t Timeout to use\n"
            "  -p Pretimeout to use (is needed to save the reset reason)\n"
            "  -h show this message\n\n");
}

int main(int argc, char** argv)
{
    unsigned int pretimeout = 1;
    unsigned int timeout = 10;
    char *watchdog_character_device = 0;
    int c;
    int ret;

    opterr = 0;
    while ((c = getopt (argc, argv, "d:t:p:h")) != -1) {
        switch (c)
        {
        case 't':
            sscanf(optarg, "%u", &timeout);
            break;
        case 'p':
            sscanf(optarg, "%u", &pretimeout);
            break;
        case 'd':
            watchdog_character_device = optarg;
            break;
        case 'h':
            usage();
            return 1;
        default:
            break;
        }
    }

    if (watchdog_character_device == 0) {
        printf("Please specify a device\n");
        usage();
        return 3;
    }

    int fd = open(watchdog_character_device, O_RDWR);

    ret = ioctl(fd, WDIOC_SETTIMEOUT, &timeout);
    if (ret)
        printf("Could not set watchdog timeout to %u\n", timeout);

    ret = ioctl(fd, WDIOC_SETPRETIMEOUT, &pretimeout);
    if (ret)
        printf("Could not set watchdog pretimeout to %u\n", pretimeout);

    /* This would be the call to send keep alives to the watchdog */
    ret = ioctl(fd, WDIOC_KEEPALIVE, 0);
    if (ret)
        printf("Could not trigger watchdog\n");


    for (unsigned int i = 0; i < timeout; i++) {
        printf("%d seconds until watchdog triggers\n", timeout-i);
        printf("%d seconds until pretimeout triggers\n", timeout-pretimeout-i);
        sleep(1);
    }

    close(fd);

    return 0;
}

