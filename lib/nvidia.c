/* Copyright (c) 2011 Amir Aupov <fads93@gmail.com>
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
#include <X11/Xlib.h>
#include <NVCtrl/NVCtrl.h>
#include <NVCtrl/NVCtrlLib.h>
#include <nvidia.h>
#include <types.h>
#include <sensors-interface-common.h>
#include <middlelayer.h>

/* Gtk/Glib includes */
#include <glib.h>

/* Global includes */
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

/* Global variables */
Display *nvidia_sensors_display;

/* Local functions */
int read_gpus (t_chip *chip);

/* Defines */
#define ZERO_KELVIN -273

int initialize_nvidia (GPtrArray *chips)
{
    int retval;
    int num_gpus;
    t_chip *chip;
    t_chipfeature *chipfeature;

    TRACE ("enters initialize_nvidia");

    chip = g_new0 (t_chip, 1);
    //chip -> chip_name = g_strdup(_("nvidia"));
    chip -> chip_features = g_ptr_array_new ();
    chip -> num_features = 0;
    chip -> description = g_strdup(_("NVidia GPU core temperature"));
    chip -> name = g_strdup(_("nvidia"));
    chip -> sensorId = g_strdup("nvidia");
    chip -> type = GPU;

    num_gpus = read_gpus (chip);
    if (chip -> num_features > 0) {
        int i;
        for (i = 0; i < num_gpus; i++) {
            chipfeature = g_ptr_array_index (chip -> chip_features, i);
            g_assert (chipfeature != NULL);
            chipfeature -> address = i;
            chipfeature -> name = g_strdup(chipfeature -> devicename);
            chipfeature -> color = g_strdup ("#000000");
            chipfeature -> valid = TRUE;
            //chipfeature -> formatted_value = g_strdup ("0.0");
            chipfeature -> raw_value = 0.0;
            chipfeature -> class = TEMPERATURE;
            chipfeature -> min_value = 10.0;
            chipfeature -> max_value = 50.0;
            chipfeature -> show = FALSE;
        }
        g_ptr_array_add (chips, chip);
        retval = 2;
    }
    else
        retval = 0;

    TRACE ("leaves initialize_nvidia");

    return retval;
}


double get_nvidia_value (int gpu)
{
    int temp;

    TRACE ("enters get_nvidia_value for %d gpu", gpu);

    if (!(XNVCTRLQueryTargetAttribute (nvidia_sensors_display,
                                       NV_CTRL_TARGET_TYPE_GPU,
                                       gpu,
                                       0,
                                       NV_CTRL_GPU_CORE_TEMPERATURE,
                                       &temp))) {
        TRACE ("NVCtrl doesn't work properly");
        return ZERO_KELVIN;
    }

    TRACE ("leaves get_nvidia_value for %d gpu", gpu);

    return (double) (1.0 * temp);
}


void refresh_nvidia (gpointer chip_feature, gpointer data)
{
    t_chipfeature *cf;
    double value;

    g_assert (chip_feature != NULL);

    TRACE ("enters refresh_nvidia");

    cf = (t_chipfeature *) chip_feature;
    value = get_nvidia_value (cf -> address);
    if (value == ZERO_KELVIN)
        return;

    //g_free (cf -> formatted_value);
    //cf -> formatted_value = g_strdup_printf(_("%.1f °C"), value);
    cf -> raw_value = value;

    TRACE ("leaves refresh_nvidia");
}


int read_gpus (t_chip *chip)
{
    t_chipfeature *chipfeature;
    int num_gpus;
    int event, error;
    int i;

    TRACE ("enters read_gpus");

    /* create the connection to the X server */
    if (!(nvidia_sensors_display = XOpenDisplay (NULL))) {
        TRACE ("failed to connect to X server");
        return 0;
    }

    /* check if the NVCtrl is available on this X server
     * if so - add sensors*/
    if (!(XNVCTRLQueryExtension (nvidia_sensors_display,
                &event, &error))) {
        TRACE ("NVCtrl is not available");
        return 0;
    }

    if (!(XNVCTRLQueryTargetCount (nvidia_sensors_display,
            NV_CTRL_TARGET_TYPE_GPU,
            &num_gpus))) {
        TRACE ("No NVidia devices found");
        return 0;
    }

    for (i = 0; i < num_gpus; i++) {
        gchar *device_name = (gchar*) malloc (100 * sizeof(gchar));
        if (XNVCTRLQueryTargetStringAttribute (nvidia_sensors_display,
                                               NV_CTRL_TARGET_TYPE_GPU,
                                               i,
                                               0,
                                               NV_CTRL_STRING_PRODUCT_NAME,
                                               &device_name))
            TRACE ("GPU%d:%s", i, device_name);

        chipfeature = g_new0 (t_chipfeature, 1);
        chipfeature -> devicename = g_strdup (device_name);
        chipfeature -> name = g_strdup (device_name);
        g_ptr_array_add (chip -> chip_features, chipfeature);
        chip -> num_features++;
    }

    TRACE ("leaves read_gpus");
    return num_gpus;
}
