/* nvidia.cc
 * Part of xfce4-sensors-plugin
 *
 * Copyright (c) 2011 Amir Aupov <fads93@gmail.com>
 * Copyright (c) 2017 Fabian Nowak <timystery@arcor.de>
 * Copyright (c) 2021 Jan Ziak <0xe2.0x9a.0x9b@xfce.org>
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

/* The fixes file has to be included before any other #include directives */
#include "xfce4++/util/fixes.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include "xfce4++/util.h"

#include <X11/Xlib.h> /* Must be before NVCtrl includes */
#include <NVCtrl/NVCtrl.h>
#include <NVCtrl/NVCtrlLib.h>
#include <nvidia.h>

/* Package includes */
#include <middlelayer.h>
#include <sensors-interface-common.h>
#include <types.h>

/* Global variables */
Display *nvidia_sensors_display;

/* Local functions */
static int read_gpus (const Ptr<t_chip> &chip);

/* -------------------------------------------------------------------------- */
int
initialize_nvidia (std::vector<Ptr<t_chip>> &chips)
{
    int retval;

    /*
     * According to "Brand Guidelines for the NVIDIA Partner Network" PDF, May 2020:
     * Always write NVIDIA with all caps, not nvidia nor NVidia.
     */

    auto chip = xfce4::make<t_chip>();
    chip->description = _("NVIDIA GPU core temperature");
    chip->name = _("nvidia");
    chip->sensorId = "nvidia";
    chip->type = GPU;

    read_gpus (chip);
    if (!chip->chip_features.empty()) {
        for (size_t i = 0; i < chip->chip_features.size(); i++) {
            auto feature = chip->chip_features[i];
            feature->address = i;
            feature->name = feature->devicename;
            feature->color_orEmpty = "";
            feature->valid = true;
            feature->raw_value = 0.0;
            feature->cls = TEMPERATURE;
            feature->min_value = 20.0;
            feature->max_value = 80.0;
            feature->show = false;
        }
        chips.push_back(chip);
        retval = 2;
    }
    else {
        retval = 0;
    }

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
refresh_nvidia (const Ptr<t_chipfeature> &feature)
{
    gdouble value = get_nvidia_value (feature->address);
    if (value != ZERO_KELVIN)
        feature->raw_value = value;
}


/* -------------------------------------------------------------------------- */
static int
read_gpus (const Ptr<t_chip> &chip)
{
    int num_gpus = 0;

    /* create the connection to the X server */
    nvidia_sensors_display = XOpenDisplay (NULL);
    if (nvidia_sensors_display) {
        int event, error;

        /* check if the NVCtrl is available on this X server
         * if so - add sensors*/
        if (XNVCTRLQueryExtension (nvidia_sensors_display, &event, &error)) {
            XNVCTRLQueryTargetCount (nvidia_sensors_display,
                NV_CTRL_TARGET_TYPE_GPU,
                &num_gpus);
        }
    }

    for (int idx_gpu = 0; idx_gpu < num_gpus; idx_gpu++) {
        gchar *gpuname = NULL; /* allocated by libxnvctrl */

        auto feature = xfce4::make<t_chipfeature>();

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
            feature->devicename = xfce4::sprintf ("GPU %d", idx_gpu);
        }
        feature->name = feature->devicename;

        chip->chip_features.push_back(feature);
    }

    return num_gpus;
}
