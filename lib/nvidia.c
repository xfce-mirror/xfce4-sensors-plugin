/* File: nvidia.c
 *
 * Copyright (c) 2011 Amir Aupov <fads93@gmail.com>
 * Copyright (c) 2017 Fabian Nowak <timystery@arcor.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* Package includes */
#include <types.h>
#include <sensors-interface-common.h>
#include <middlelayer.h>

/* Gtk/Glib includes */
#include <glib.h>

/* Global includes */
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include <X11/Xlib.h> /* Must be before NVCtrl includes */
#include <NVCtrl/NVCtrl.h>
#include <NVCtrl/NVCtrlLib.h>
#include <nvidia.h>

/* Global variables */
Display *nvidia_sensors_display;

/* Local functions */
int read_gpus (t_chip *chip);

/* -------------------------------------------------------------------------- */
int
initialize_nvidia (GPtrArray *arr_ptr_chips)
{
    int retval;
    int num_gpus;
    t_chip *ptr_chip;
    t_chipfeature *ptr_chipfeature;

    g_assert (arr_ptr_chips != NULL);

    ptr_chip = g_new0 (t_chip, 1);
    ptr_chip->chip_features = g_ptr_array_new ();
    ptr_chip->num_features = 0;
    ptr_chip->description = g_strdup(_("NVidia GPU core temperature"));
    ptr_chip->name = g_strdup(_("nvidia"));
    ptr_chip->sensorId = g_strdup("nvidia");
    ptr_chip->type = GPU;

    num_gpus = read_gpus (ptr_chip);
    if (ptr_chip->num_features > 0) {
        int i;
        for (i = 0; i < num_gpus; i++) {
            ptr_chipfeature = g_ptr_array_index (ptr_chip->chip_features, i);
            g_assert (ptr_chipfeature != NULL);
            ptr_chipfeature->address = i;
            ptr_chipfeature->name = g_strdup(ptr_chipfeature->devicename);
            ptr_chipfeature->color = g_strdup ("#000000");
            ptr_chipfeature->valid = TRUE;
            ptr_chipfeature->raw_value = 0.0;
            ptr_chipfeature->class = TEMPERATURE;
            ptr_chipfeature->min_value = 10.0;
            ptr_chipfeature->max_value = 70.0;
            ptr_chipfeature->show = FALSE;
        }
        g_ptr_array_add (arr_ptr_chips, ptr_chip);
        retval = 2;
    }
    else
        retval = 0;

    return retval;
}


/* -------------------------------------------------------------------------- */
double
get_nvidia_value (int idx_gpu)
{
    int val_temperature = 0;
    double result = ZERO_KELVIN;

    if (XNVCTRLQueryTargetAttribute (nvidia_sensors_display,
                                     NV_CTRL_TARGET_TYPE_GPU,
                                     idx_gpu,
                                     0,
                                     NV_CTRL_GPU_CORE_TEMPERATURE,
                                     &val_temperature)) {
        result = (double) (1.0 * val_temperature);
    }

    return result;
}


/* -------------------------------------------------------------------------- */
void
refresh_nvidia (gpointer ptr_chipfeature, gpointer ptr_unused)
{
    t_chipfeature *ptr_localchipfeature;
    double value;

    ptr_localchipfeature = (t_chipfeature *) ptr_chipfeature;
    g_assert (ptr_localchipfeature != NULL);

    value = get_nvidia_value (ptr_localchipfeature->address);
    if (value != ZERO_KELVIN)
        ptr_localchipfeature->raw_value = value;
}


/* -------------------------------------------------------------------------- */
int
read_gpus (t_chip *ptr_chip)
{
    t_chipfeature *ptr_chipfeature;
    int num_gpus = 0;
    int event, error;
    int idx_gpu;

    g_assert (ptr_chip != NULL);

    /* create the connection to the X server */
    nvidia_sensors_display = XOpenDisplay (NULL);
    if (nvidia_sensors_display) {

        /* check if the NVCtrl is available on this X server
         * if so - add sensors*/
        if (XNVCTRLQueryExtension (nvidia_sensors_display, &event, &error)) {
            XNVCTRLQueryTargetCount (nvidia_sensors_display,
                NV_CTRL_TARGET_TYPE_GPU,
                &num_gpus);
        }
    }

    for (idx_gpu = 0; idx_gpu < num_gpus; idx_gpu++) {
        gchar* ptr_str_gpuname = NULL; /* allocated by libxnvctrl */
        ptr_chipfeature = g_new0 (t_chipfeature, 1);

        if (XNVCTRLQueryTargetStringAttribute (nvidia_sensors_display,
                                               NV_CTRL_TARGET_TYPE_GPU,
                                               idx_gpu,
                                               0,
                                               NV_CTRL_STRING_PRODUCT_NAME,
                                               &ptr_str_gpuname)) {
            g_assert (ptr_str_gpuname != NULL);
            ptr_chipfeature->devicename = ptr_str_gpuname;  /* "it is the caller's responsibility to free ..." */
        }
        else
        {
            ptr_chipfeature->devicename = g_strdup_printf ("GPU %d", idx_gpu);
        }
        ptr_chipfeature->name = g_strdup (ptr_chipfeature->devicename);

        g_ptr_array_add (ptr_chip->chip_features, ptr_chipfeature);
        ptr_chip->num_features++;
    }

    return num_gpus;
}
