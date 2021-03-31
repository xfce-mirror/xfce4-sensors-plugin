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

#include <glib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/* Package includes */
#include <middlelayer.h>
#include <sensors-interface-common.h>
#include <types.h>

#include <X11/Xlib.h> /* Must be before NVCtrl includes */
#include <NVCtrl/NVCtrl.h>
#include <NVCtrl/NVCtrlLib.h>
#include <nvidia.h>

/* Global variables */
Display *nvidia_sensors_display;

/* Local functions */
static int read_gpus (t_chip *chip);

/* -------------------------------------------------------------------------- */
int
initialize_nvidia (GPtrArray *chips)
{
    int num_gpus, retval;
    t_chip *chip;
    t_chipfeature *feature;

    g_assert (chips != NULL);

    /*
     * According to "Brand Guidelines for the NVIDIA Partner Network" PDF, May 2020:
     * Always write NVIDIA with all caps, not nvidia nor NVidia.
     */

    chip = g_new0 (t_chip, 1);
    chip->chip_features = g_ptr_array_new ();
    chip->num_features = 0;
    chip->description = g_strdup (_("NVIDIA GPU core temperature"));
    chip->name = g_strdup (_("nvidia"));
    chip->sensorId = g_strdup ("nvidia");
    chip->type = GPU;

    num_gpus = read_gpus (chip);
    if (chip->num_features > 0) {
        int i;
        for (i = 0; i < num_gpus; i++) {
            feature = g_ptr_array_index (chip->chip_features, i);
            g_assert (feature != NULL);
            feature->address = i;
            feature->name = g_strdup (feature->devicename);
            feature->color = g_strdup ("#000000");
            feature->valid = TRUE;
            feature->raw_value = 0.0;
            feature->class = TEMPERATURE;
            feature->min_value = 10.0;
            feature->max_value = 70.0;
            feature->show = FALSE;
        }
        g_ptr_array_add (chips, chip);
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
    int temperature = 0;
    double result = ZERO_KELVIN;

    if (XNVCTRLQueryTargetAttribute (nvidia_sensors_display,
                                     NV_CTRL_TARGET_TYPE_GPU,
                                     idx_gpu,
                                     0,
                                     NV_CTRL_GPU_CORE_TEMPERATURE,
                                     &temperature)) {
        result = temperature;
    }

    return result;
}


/* -------------------------------------------------------------------------- */
void
refresh_nvidia (gpointer chip_feature, gpointer unused)
{
    t_chipfeature *feature;
    double value;

    feature = (t_chipfeature *) chip_feature;
    g_assert (feature != NULL);

    value = get_nvidia_value (feature->address);
    if (value != ZERO_KELVIN)
        feature->raw_value = value;
}


/* -------------------------------------------------------------------------- */
static int
read_gpus (t_chip *chip)
{
    t_chipfeature *feature;
    int num_gpus = 0;
    int event, error;
    int idx_gpu;

    g_assert (chip != NULL);

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
        gchar* gpuname = NULL; /* allocated by libxnvctrl */
        feature = g_new0 (t_chipfeature, 1);

        if (XNVCTRLQueryTargetStringAttribute (nvidia_sensors_display,
                                               NV_CTRL_TARGET_TYPE_GPU,
                                               idx_gpu,
                                               0,
                                               NV_CTRL_STRING_PRODUCT_NAME,
                                               &gpuname)) {
            g_assert (gpuname != NULL);
            feature->devicename = gpuname;  /* "it is the caller's responsibility to free ..." */
        }
        else
        {
            feature->devicename = g_strdup_printf ("GPU %d", idx_gpu);
        }
        feature->name = g_strdup (feature->devicename);

        g_ptr_array_add (chip->chip_features, feature);
        chip->num_features++;
    }

    return num_gpus;
}
