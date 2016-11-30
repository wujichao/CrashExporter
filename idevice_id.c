/*
 * idevice_id.c
 * Prints device name or a list of attached devices
 *
 * Copyright (C) 2010 Nikias Bassen <nikias@gmx.li>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libimobiledevice/libimobiledevice.h>
#include <libimobiledevice/lockdown.h>

/*
 * input:
 * output: udid, name
 */
int get_current_device(char *udid, char *name)
{
	idevice_t device = NULL;
	lockdownd_client_t client = NULL;
	char **dev_list = NULL;
    char *device_name = NULL;
	int ret = 0;
	int i;

    if (idevice_get_device_list(&dev_list, &i) < 0) {
        fprintf(stderr, "ERROR: Unable to retrieve device list!\n");
        return -1;
    }

    for (i = 0; dev_list[i] != NULL; i++) {
        printf("%s\n", dev_list[i]);
    }

    if (i == 1) {
        strcpy (udid, dev_list[0]);
    }

    idevice_device_list_free(dev_list);

    if (i != 1) {
        return -1;//同时连接零个或多个设备
    }

    idevice_new(&device, udid);
    if (!device) {
        fprintf(stderr, "ERROR: No device with UDID=%s attached.\n", udid);
        return -2;
    }

    if (LOCKDOWN_E_SUCCESS != lockdownd_client_new(device, &client, "idevice_id")) {
        idevice_free(device);
        fprintf(stderr, "ERROR: Connecting to device failed!\n");
        return -2;
    }

    if ((LOCKDOWN_E_SUCCESS != lockdownd_get_device_name(client, &device_name)) || !device_name) {
        fprintf(stderr, "ERROR: Could not get device name!\n");
        ret = -2;
    }

    lockdownd_client_free(client);
    idevice_free(device);

    if (ret == 0) {
        printf("%s\n", device_name);
    }

    strcpy(name, device_name);

    if (device_name) {
        free(device_name);
    }

    return ret;
}

/*
 * input: udid
 * output: name
 */
int get_device_name(const char *udid, char *device_name)
{
    int res = -1;

    idevice_t device = NULL;
    if (idevice_new(&device, udid) != IDEVICE_E_SUCCESS) {
        fprintf(stderr, "ERROR: Could not connect to device\n");
        return -1;
    }

    lockdownd_client_t lockdown = NULL;
    lockdownd_error_t lerr = lockdownd_client_new_with_handshake(device, &lockdown, "idevicename");
    if (lerr != LOCKDOWN_E_SUCCESS) {
        idevice_free(device);
        fprintf(stderr, "ERROR: Could not connect to lockdownd, error code %d\n", lerr);
        return -1;
    }

    // getting device name
    char* name = NULL;
    lerr = lockdownd_get_device_name(lockdown, &name);
    if (name) {
        printf("%s\n", name);
        strcpy(device_name, name);
        free(name);
        res = 0;
    } else {
        fprintf(stderr, "ERROR: Could not get device name, lockdown error %d\n", lerr);
        res = -1;
    }

    lockdownd_client_free(lockdown);
    idevice_free(device);

    return res;
}
