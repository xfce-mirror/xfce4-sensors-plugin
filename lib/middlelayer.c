/* File: middlelayer.c
 *
 *      Copyright 2006-2017 Fabian Nowak <timytery@arcor.de>
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif


#ifdef HAVE_LINUX
#include <sys/utsname.h>
#endif

/* Gtk/Glib includes */
#include <glib.h>

/* Global includes */
#include <string.h>

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
initialize_all (GPtrArray **outptr_arr_ptr_chips, gboolean *outptr_suppressmessage)
{
    int result = 0;

    TRACE ("enters initialize_all");

    g_assert (outptr_arr_ptr_chips != NULL);

    *outptr_arr_ptr_chips = g_ptr_array_new(); //_with_free_func(free_chip);

     #ifdef HAVE_LIBSENSORS
    result += initialize_libsensors (*outptr_arr_ptr_chips);
    #endif

    #ifdef HAVE_HDDTEMP
    result += initialize_hddtemp (*outptr_arr_ptr_chips, outptr_suppressmessage);
    #endif

    #ifdef HAVE_ACPI
    result += initialize_ACPI (*outptr_arr_ptr_chips);
    #endif

    #ifdef HAVE_NVIDIA
    result += initialize_nvidia (*outptr_arr_ptr_chips);
    #endif

    TRACE ("leaves initialize_all, chips->len=%d", (*outptr_arr_ptr_chips)->len);

    return result;
}


/* -------------------------------------------------------------------------- */
void
refresh_chip (gpointer ptr_chip, gpointer ptr_data)
{
    t_chip *ptr_chip_structure;

    TRACE ("enters refresh_chip");

    g_assert (ptr_chip != NULL);

    ptr_chip_structure = (t_chip*) ptr_chip;

    switch (ptr_chip_structure->type)
    {
    #ifdef HAVE_ACPI
        case ACPI: {
            g_ptr_array_foreach (ptr_chip_structure->chip_features, refresh_acpi, NULL);
            break;
        }
    #endif

    #ifdef HAVE_LIBSENSORS
        case LMSENSOR: {
            g_ptr_array_foreach (ptr_chip_structure->chip_features, refresh_lmsensors, NULL);
            break;
        }
    #endif

    #ifdef HAVE_HDDTEMP
        case HDD: {
            g_assert (ptr_data != NULL);
            g_ptr_array_foreach (ptr_chip_structure->chip_features, refresh_hddtemp, ptr_data); /* note that data is of *t_sensors! */
            break;
        }
    #endif

    #ifdef HAVE_NVIDIA
        case GPU: {
            g_ptr_array_foreach (ptr_chip_structure->chip_features, refresh_nvidia, NULL);
            break;
        }
    #endif

        default: {
            break;
        }
    }

    TRACE ("leaves refresh_chip");
}


/* -------------------------------------------------------------------------- */
void
refresh_all_chips (GPtrArray *arr_ptr_chips, t_sensors *ptr_sensors )
{
    TRACE ("enters refresh_all_chips");

    g_assert (arr_ptr_chips != NULL);
    g_assert (ptr_sensors != NULL);

    g_ptr_array_foreach (arr_ptr_chips, refresh_chip, ptr_sensors);

    TRACE ("leaves refresh_all_chips");
}


/* -------------------------------------------------------------------------- */
void
categorize_sensor_type (t_chipfeature* ptr_chipfeature)
{
    TRACE ("enters categorize_sensor_type");
    g_assert (ptr_chipfeature != NULL);

    if ( strstr(ptr_chipfeature->name, "Temp")!=NULL
    || strstr(ptr_chipfeature->name, "temp")!=NULL ) {
        ptr_chipfeature->class = TEMPERATURE;
        ptr_chipfeature->min_value = 0.0;
        ptr_chipfeature->max_value = 80.0;
    } else if ( strstr(ptr_chipfeature->name, "VCore")!=NULL
    || strstr(ptr_chipfeature->name, "3V")!=NULL
    || strstr(ptr_chipfeature->name, "5V")!=NULL
    || strstr(ptr_chipfeature->name, "12V")!=NULL ) {
        ptr_chipfeature->class = VOLTAGE;
        ptr_chipfeature->min_value = 1.0;
        ptr_chipfeature->max_value = 12.2;
    } else if ( strstr(ptr_chipfeature->name, "Fan")!=NULL
    || strstr(ptr_chipfeature->name, "fan")!=NULL ) {
        ptr_chipfeature->class = SPEED;
        ptr_chipfeature->min_value = 1000.0;
        ptr_chipfeature->max_value = 3500.0;
    } else if ( strstr(ptr_chipfeature->name, "alarm")!=NULL
    || strstr(ptr_chipfeature->name, "Alarm")!=NULL ) {
        ptr_chipfeature->class = STATE;
        ptr_chipfeature->min_value = 0.0;
        ptr_chipfeature->max_value = 1.0;
    } else {
        ptr_chipfeature->class = OTHER;
        ptr_chipfeature->min_value = 0.0;
        ptr_chipfeature->max_value = 7000.0;
    }

    TRACE ("leaves categorize_sensor_type");
}


/* -------------------------------------------------------------------------- */
int
sensor_get_value (t_chip *ptr_chip, int idx_chipfeature, double *outptr_value, gboolean *outptr_suppressmessage)
{
    t_chipfeature *ptr_feature;
    #ifdef HAVE_HDDTEMP
        gboolean *ptr_suppress = outptr_suppressmessage;
        g_assert (ptr_suppress != NULL);
    #endif

    g_assert (ptr_chip != NULL);
    g_assert (outptr_value != NULL);

    switch (ptr_chip->type) {
        case LMSENSOR: {
            #ifdef HAVE_LIBSENSORS
                return sensors_get_feature_wrapper (ptr_chip->chip_name, idx_chipfeature, outptr_value);
            #else
                return -1;
            #endif
            break;
        }
        case HDD: {
            #ifdef HAVE_HDDTEMP
                g_assert (idx_chipfeature < ptr_chip->num_features);
                ptr_feature = (t_chipfeature *) g_ptr_array_index (ptr_chip->chip_features, idx_chipfeature);
                g_assert (ptr_feature != NULL);
                *outptr_value = get_hddtemp_value (ptr_feature->devicename, ptr_suppress);
                if (*outptr_value==NO_VALID_HDDTEMP_PROGRAM) {
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
                g_assert (idx_chipfeature < ptr_chip->num_features);
                ptr_feature = (t_chipfeature *) g_ptr_array_index (ptr_chip->chip_features, idx_chipfeature);
                g_assert (ptr_feature != NULL);
                /* TODO: seperate refresh from get operation! */
                refresh_acpi ((gpointer) ptr_feature, NULL);
                *outptr_value = ptr_feature->raw_value;
                return 0;
            #else
                return -1;
            #endif
            break;
        }
        case GPU: {
            #ifdef HAVE_NVIDIA
                g_assert (idx_chipfeature < ptr_chip->num_features);
                ptr_feature = (t_chipfeature *) g_ptr_array_index (ptr_chip->chip_features, idx_chipfeature);
                g_assert (ptr_feature != NULL);
                /* TODO: seperate refresh from get operation! */
                refresh_nvidia ((gpointer) ptr_feature, NULL);
                *outptr_value = ptr_feature->raw_value;
                return 0;
            #else
                return -1;
            #endif
            break;
        }
        default: {
            ptr_feature = NULL;
            return -1;
            break;
        }
    }

    return -1;
}


/* -------------------------------------------------------------------------- */
void
free_chipfeature (gpointer ptr_chipfeature, gpointer ptr_unused)
{
    t_chipfeature *ptr_localchipfeature = (t_chipfeature *) ptr_chipfeature;
    g_assert (ptr_localchipfeature!=NULL);

    g_free (ptr_localchipfeature->name);
    g_free (ptr_localchipfeature->devicename);
    g_free (ptr_localchipfeature->formatted_value);
    g_free (ptr_localchipfeature->color);
    g_free (ptr_localchipfeature);
}


/* -------------------------------------------------------------------------- */
void
free_chip (gpointer ptr_chip, gpointer ptr_unused)
{
    t_chip *ptr_chip_structure = (t_chip *) ptr_chip;
    g_assert (ptr_chip_structure != NULL);

    g_free (ptr_chip_structure->description);
    g_free (ptr_chip_structure->name);
    g_free (ptr_chip_structure->sensorId);

#ifdef HAVE_LIBSENSORS
    if (ptr_chip_structure->type==LMSENSOR) {
        free_lmsensors_chip (ptr_chip);
    }
#endif
#ifdef HAVE_ACPI
    if (ptr_chip_structure->type==ACPI) {
        free_acpi_chip (ptr_chip);
    }
#endif
//#ifdef HAVE_HDDTEMP
    //if (ptr_chip_structure->type==HDD) {
        //free_hddtemp_chip (chip);
    //}
//#endif

    g_ptr_array_foreach (ptr_chip_structure->chip_features, free_chipfeature, NULL);
    g_ptr_array_free (ptr_chip_structure->chip_features, TRUE);
    g_free (ptr_chip_structure->chip_name);
    g_free(ptr_chip_structure);
}


/* -------------------------------------------------------------------------- */
void
cleanup_interfaces (void)
{
#ifdef HAVE_LIBSENSORS
    sensors_cleanup();
#endif
}
