/* middlelayer.cc
 * Part of xfce4-sensors-plugin
 *
 * Copyright (c) 2006-2017 Fabian Nowak <timystery@arcor.de>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

/* The fixes file has to be included before any other #include directives */
#include "xfce4++/util/fixes.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <string.h>

#ifdef HAVE_LINUX
#include <sys/utsname.h>
#endif

/* Package includes */
#include <middlelayer.h>
#include <sensors-interface-common.h>

#ifdef HAVE_LIBSENSORS
    #include <lmsensors.h>
#endif
#ifdef HAVE_HDDTEMP
    #include <hddtemp.h>
#endif
#ifdef HAVE_ACPI
    #include <acpi.h>
#endif
#ifdef HAVE_NVIDIA
    #include <nvidia.h>
#endif


/* -------------------------------------------------------------------------- */
int
initialize_all (std::vector<Ptr<t_chip>> &chips, bool *out_suppressmessage)
{
    int result = 0;

    chips.clear();

     #ifdef HAVE_LIBSENSORS
    result += initialize_libsensors (chips);
    #endif

    #ifdef HAVE_HDDTEMP
    result += initialize_hddtemp (chips, out_suppressmessage);
    #endif

    #ifdef HAVE_ACPI
    result += initialize_ACPI (chips);
    #endif

    #ifdef HAVE_NVIDIA
    result += initialize_nvidia (chips);
    #endif

    return result;
}


/* -------------------------------------------------------------------------- */
void
refresh_chip (const Ptr<t_chip> &chip, t_sensors *data)
{
    switch (chip->type)
    {
    #ifdef HAVE_ACPI
        case ACPI:
            for (const auto &feature : chip->chip_features)
                refresh_acpi (feature);
            break;
    #endif

    #ifdef HAVE_LIBSENSORS
        case LMSENSOR:
            for (const auto &feature : chip->chip_features)
                refresh_lmsensors (feature);
            break;
    #endif

    #ifdef HAVE_HDDTEMP
        case HDD:
            g_assert (data != NULL);
            for (const auto &feature : chip->chip_features)
                refresh_hddtemp (feature, data); /* note that data is of *t_sensors! */
            break;
    #endif

    #ifdef HAVE_NVIDIA
        case GPU:
            for (const auto &feature : chip->chip_features)
                refresh_nvidia (feature);
            break;
    #endif

        default:;
    }
}


/* -------------------------------------------------------------------------- */
void
refresh_all_chips (const std::vector<Ptr<t_chip>> &chips, t_sensors *sensors)
{
    g_assert (sensors != NULL);

    for (auto chip : chips)
        refresh_chip (chip, sensors);
}


/* -------------------------------------------------------------------------- */
void
categorize_sensor_type (const Ptr<t_chipfeature> &feature)
{
    const char *name = feature->name.c_str();

    if (strstr(name, "Temp") || strstr(name, "temp") || strstr(name, "thermal"))
    {
        feature->cls = TEMPERATURE;
        feature->min_value = 0.0;
        feature->max_value = 80.0;
    }
    else if (strstr(name, "VCore") || strstr(name, "3V") || strstr(name, "5V") || strstr(name, "12V"))
    {
        feature->cls = VOLTAGE;
        feature->min_value = 1.0;
        feature->max_value = 12.2;
    }
    else if (strstr(name, "Fan") || strstr(name, "fan"))
    {
        feature->cls = SPEED;
        feature->min_value = 1000.0;
        feature->max_value = 3500.0;
    }
    else if (strstr(name, "alarm") || strstr(name, "Alarm"))
    {
        feature->cls = STATE;
        feature->min_value = 0.0;
        feature->max_value = 1.0;
    }
    else if (strstr(name, "power") || strstr(name, "Power"))
    {
        feature->cls = POWER;
        feature->min_value = 0.0;
        feature->max_value = 1.0;
    }
    else if (strstr(name, "current") || strstr (name, "Current"))
    {
        feature->cls = CURRENT;
        feature->min_value = 0.0;
        feature->max_value = 1.0;
    }
    else
    {
        feature->cls = OTHER;
        feature->min_value = 0.0;
        feature->max_value = 7000.0;
    }
}


/* -------------------------------------------------------------------------- */
int
sensor_get_value (const Ptr<t_chip> &chip, size_t idx_chipfeature, double *out_value, bool *out_suppressmessage)
{
    #ifdef HAVE_HDDTEMP
        bool *suppress = out_suppressmessage;
        g_assert (suppress != NULL);
    #endif

    g_assert (out_value != NULL);

    switch (chip->type) {
        case LMSENSOR: {
            #ifdef HAVE_LIBSENSORS
                return sensors_get_value (chip->chip_name, idx_chipfeature, out_value);
            #else
                return -1;
            #endif
            break;
        }
        case HDD: {
            #ifdef HAVE_HDDTEMP
                g_assert (idx_chipfeature < chip->chip_features.size());
                auto feature = chip->chip_features[idx_chipfeature];
                *out_value = get_hddtemp_value (feature->devicename, suppress);
                if (*out_value==NO_VALID_HDDTEMP_PROGRAM) {
                    return NO_VALID_HDDTEMP_PROGRAM;
                }
                return 0;
            #else
                return -1;
            #endif
            break;
        }
        case ACPI: {
            #ifdef HAVE_ACPI
                g_assert (idx_chipfeature < chip->chip_features.size());
                auto feature = chip->chip_features[idx_chipfeature];
                /* TODO: seperate refresh from get operation! */
                refresh_acpi (feature);
                *out_value = feature->raw_value;
                return 0;
            #else
                return -1;
            #endif
            break;
        }
        case GPU: {
            #ifdef HAVE_NVIDIA
                g_assert (idx_chipfeature < chip->chip_features.size());
                auto feature = chip->chip_features[idx_chipfeature];
                /* TODO: seperate refresh from get operation! */
                refresh_nvidia (feature);
                *out_value = feature->raw_value;
                return 0;
            #else
                return -1;
            #endif
            break;
        }
        default: {
            return -1;
        }
    }

    return -1;
}


/* -------------------------------------------------------------------------- */
t_chip::~t_chip()
{
    g_info ("%s", __PRETTY_FUNCTION__);

#ifdef HAVE_LIBSENSORS
    if (type == LMSENSOR)
        free_lmsensors_chip (this);
#endif

#ifdef HAVE_ACPI
    if (type == ACPI)
        free_acpi_chip (this);
#endif

//#ifdef HAVE_HDDTEMP
    //if (type == HDD)
        //free_hddtemp_chip (this);
//#endif

    g_free (chip_name);
}


/* -------------------------------------------------------------------------- */
void
cleanup_interfaces ()
{
#ifdef HAVE_LIBSENSORS
    sensors_cleanup();
#endif
}
