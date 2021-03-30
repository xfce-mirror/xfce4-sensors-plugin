/* File: middlelayer.c
 *
 * Copyright 2006-2017 Fabian Nowak <timytery@arcor.de>
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
initialize_all (GPtrArray **chips, gboolean *out_suppressmessage)
{
    int result = 0;

    g_assert (chips != NULL);

    *chips = g_ptr_array_new(); //_with_free_func(free_chip);

     #ifdef HAVE_LIBSENSORS
    result += initialize_libsensors (*chips);
    #endif

    #ifdef HAVE_HDDTEMP
    result += initialize_hddtemp (*chips, out_suppressmessage);
    #endif

    #ifdef HAVE_ACPI
    result += initialize_ACPI (*chips);
    #endif

    #ifdef HAVE_NVIDIA
    result += initialize_nvidia (*chips);
    #endif

    return result;
}


/* -------------------------------------------------------------------------- */
void
refresh_chip (gpointer ptr_chip, gpointer data)
{
    t_chip *chip;

    g_assert (ptr_chip != NULL);

    chip = (t_chip*) ptr_chip;

    switch (chip->type)
    {
    #ifdef HAVE_ACPI
        case ACPI:
            g_ptr_array_foreach (chip->chip_features, refresh_acpi, NULL);
            break;
    #endif

    #ifdef HAVE_LIBSENSORS
        case LMSENSOR:
            g_ptr_array_foreach (chip->chip_features, refresh_lmsensors, NULL);
            break;
    #endif

    #ifdef HAVE_HDDTEMP
        case HDD:
            g_assert (data != NULL);
            g_ptr_array_foreach (chip->chip_features, refresh_hddtemp, data); /* note that data is of *t_sensors! */
            break;
    #endif

    #ifdef HAVE_NVIDIA
        case GPU:
            g_ptr_array_foreach (chip->chip_features, refresh_nvidia, NULL);
            break;
    #endif

        default:;
    }
}


/* -------------------------------------------------------------------------- */
void
refresh_all_chips (GPtrArray *chips, t_sensors *sensors)
{
    g_assert (chips != NULL);
    g_assert (sensors != NULL);

    g_ptr_array_foreach (chips, refresh_chip, sensors);
}


/* -------------------------------------------------------------------------- */
void
categorize_sensor_type (t_chipfeature *feature)
{
    g_assert (feature != NULL);

    if (strstr(feature->name, "Temp")!=NULL
        || strstr(feature->name, "temp")!=NULL
        || strstr(feature->name, "thermal")!=NULL)
    {
        feature->class = TEMPERATURE;
        feature->min_value = 0.0;
        feature->max_value = 80.0;
    }
    else if (strstr(feature->name, "VCore")!=NULL
        || strstr(feature->name, "3V")!=NULL
        || strstr(feature->name, "5V")!=NULL
        || strstr(feature->name, "12V")!=NULL)
    {
        feature->class = VOLTAGE;
        feature->min_value = 1.0;
        feature->max_value = 12.2;
    }
    else if (strstr(feature->name, "Fan")!=NULL
        || strstr(feature->name, "fan")!=NULL)
    {
        feature->class = SPEED;
        feature->min_value = 1000.0;
        feature->max_value = 3500.0;
    }
    else if (strstr(feature->name, "alarm")!=NULL
        || strstr(feature->name, "Alarm")!=NULL)
    {
        feature->class = STATE;
        feature->min_value = 0.0;
        feature->max_value = 1.0;
    }
    else if (strstr(feature->name, "power")!=NULL
        || strstr(feature->name, "Power")!=NULL)
    {
        feature->class = POWER;
        feature->min_value = 0.0;
        feature->max_value = 1.0;
    }
    else if (strstr(feature->name, "current")!=NULL
        || strstr (feature->name, "Current")!=NULL)
    {
        feature->class = CURRENT;
        feature->min_value = 0.0;
        feature->max_value = 1.0;
    }
    else
    {
        feature->class = OTHER;
        feature->min_value = 0.0;
        feature->max_value = 7000.0;
    }
}


/* -------------------------------------------------------------------------- */
int
sensor_get_value (t_chip *chip, int idx_chipfeature, double *out_value, gboolean *out_suppressmessage)
{
    t_chipfeature *feature;
    #ifdef HAVE_HDDTEMP
        gboolean *suppress = out_suppressmessage;
        g_assert (suppress != NULL);
    #endif

    g_assert (chip != NULL);
    g_assert (out_value != NULL);

    switch (chip->type) {
        case LMSENSOR: {
            #ifdef HAVE_LIBSENSORS
                return sensors_get_feature_wrapper (chip->chip_name, idx_chipfeature, out_value);
            #else
                return -1;
            #endif
            break;
        }
        case HDD: {
            #ifdef HAVE_HDDTEMP
                g_assert (idx_chipfeature < chip->num_features);
                feature = (t_chipfeature *) g_ptr_array_index (chip->chip_features, idx_chipfeature);
                g_assert (feature != NULL);
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
                g_assert (idx_chipfeature < chip->num_features);
                feature = (t_chipfeature *) g_ptr_array_index (chip->chip_features, idx_chipfeature);
                g_assert (feature != NULL);
                /* TODO: seperate refresh from get operation! */
                refresh_acpi ((gpointer) feature, NULL);
                *out_value = feature->raw_value;
                return 0;
            #else
                return -1;
            #endif
            break;
        }
        case GPU: {
            #ifdef HAVE_NVIDIA
                g_assert (idx_chipfeature < chip->num_features);
                feature = (t_chipfeature *) g_ptr_array_index (chip->chip_features, idx_chipfeature);
                g_assert (feature != NULL);
                /* TODO: seperate refresh from get operation! */
                refresh_nvidia ((gpointer) feature, NULL);
                *out_value = feature->raw_value;
                return 0;
            #else
                return -1;
            #endif
            break;
        }
        default: {
            feature = NULL;
            return -1;
            break;
        }
    }

    return -1;
}


/* -------------------------------------------------------------------------- */
void
free_chipfeature (gpointer chip_feature, gpointer unused)
{
    t_chipfeature *feature = (t_chipfeature *) chip_feature;
    g_assert (feature!=NULL);

    g_free (feature->name);
    g_free (feature->devicename);
    g_free (feature->formatted_value);
    g_free (feature->color);
    g_free (feature);
}


/* -------------------------------------------------------------------------- */
void
free_chip (gpointer ptr_chip, gpointer unused)
{
    t_chip *chip = (t_chip *) ptr_chip;
    g_assert (chip != NULL);

    g_free (chip->description);
    g_free (chip->name);
    g_free (chip->sensorId);

#ifdef HAVE_LIBSENSORS
    if (chip->type==LMSENSOR) {
        free_lmsensors_chip (ptr_chip);
    }
#endif
#ifdef HAVE_ACPI
    if (chip->type==ACPI) {
        free_acpi_chip (ptr_chip);
    }
#endif
//#ifdef HAVE_HDDTEMP
    //if (ptr_chip_structure->type==HDD) {
        //free_hddtemp_chip (chip);
    //}
//#endif

    g_ptr_array_foreach (chip->chip_features, free_chipfeature, NULL);
    g_ptr_array_free (chip->chip_features, TRUE);
    g_free (chip->chip_name);
    g_free(chip);
}


/* -------------------------------------------------------------------------- */
void
cleanup_interfaces (void)
{
#ifdef HAVE_LIBSENSORS
    sensors_cleanup();
#endif
}
