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
/* #include <glib/gerror.h>
#include <glib/gmem.h>
#include <glib/gmessages.h>
#include <glib/gprintf.h> */

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
initialize_all (GPtrArray **chips, gboolean *suppressmessage)
{
    int res = 0;

    TRACE ("enters initialize_all");

    *chips = g_ptr_array_new(); //_with_free_func(free_chip);

     #ifdef HAVE_LIBSENSORS
    res += initialize_libsensors (*chips);
    #endif

    #ifdef HAVE_HDDTEMP
    res += initialize_hddtemp (*chips, suppressmessage);
    #endif

    #ifdef HAVE_ACPI
    res += initialize_ACPI (*chips);
    #endif

    #ifdef HAVE_NVIDIA
    res += initialize_nvidia (*chips);
    #endif

    TRACE ("leaves initialize_all, chips->len=%d", (*chips)->len);

    return res;
}


/* -------------------------------------------------------------------------- */
void
refresh_chip (gpointer chip, gpointer data)
{
    t_chip *ptr_chip_structure;

    g_assert (chip!=NULL);

    TRACE ("enters refresh_chip");

    ptr_chip_structure = (t_chip*) chip;

    switch (ptr_chip_structure->type)
    {
    #ifdef HAVE_ACPI
        case ACPI: {
            g_ptr_array_foreach (ptr_chip_structure->chip_features, refresh_acpi, NULL );
            break;
        }
    #endif

    #ifdef HAVE_LIBSENSORS
        case LMSENSOR: {
            g_ptr_array_foreach (ptr_chip_structure->chip_features, refresh_lmsensors, NULL );
            break;
        }
    #endif

    #ifdef HAVE_HDDTEMP
        case HDD: {
            g_ptr_array_foreach (ptr_chip_structure->chip_features, refresh_hddtemp, data ); /* note that data is of *t_sensors! */
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
refresh_all_chips (GPtrArray *chips, t_sensors *sensors )
{
    TRACE ("enters refresh_all_chips");

    g_ptr_array_foreach (chips, refresh_chip, sensors);

    TRACE ("leaves refresh_all_chips");
}


/* -------------------------------------------------------------------------- */
void
categorize_sensor_type (t_chipfeature* chipfeature)
{
    TRACE ("enters categorize_sensor_type");

    if ( strstr(chipfeature->name, "Temp")!=NULL
    || strstr(chipfeature->name, "temp")!=NULL ) {
        chipfeature->class = TEMPERATURE;
        chipfeature->min_value = 0.0;
        chipfeature->max_value = 80.0;
    } else if ( strstr(chipfeature->name, "VCore")!=NULL
    || strstr(chipfeature->name, "3V")!=NULL
    || strstr(chipfeature->name, "5V")!=NULL
    || strstr(chipfeature->name, "12V")!=NULL ) {
        chipfeature->class = VOLTAGE;
        chipfeature->min_value = 1.0;
        chipfeature->max_value = 12.2;
    } else if ( strstr(chipfeature->name, "Fan")!=NULL
    || strstr(chipfeature->name, "fan")!=NULL ) {
        chipfeature->class = SPEED;
        chipfeature->min_value = 1000.0;
        chipfeature->max_value = 3500.0;
    } else if ( strstr(chipfeature->name, "alarm")!=NULL
    || strstr(chipfeature->name, "Alarm")!=NULL ) {
        chipfeature->class = STATE;
        chipfeature->min_value = 0.0;
        chipfeature->max_value = 1.0;
    } else {
        chipfeature->class = OTHER;
        chipfeature->min_value = 0.0;
        chipfeature->max_value = 7000.0;
    }

    TRACE ("leaves categorize_sensor_type");
}


/* -------------------------------------------------------------------------- */
int
sensor_get_value (t_chip *chip, int number, double *value, gboolean *suppressmessage)
{
    t_chipfeature *feature;
        #ifdef HAVE_HDDTEMP
    gboolean *suppress = suppressmessage;
        #endif
    /* TRACE ("enters sensor_get_value %d", number); */

    g_assert (chip!=NULL);

    switch (chip->type) {
        case LMSENSOR: {
            #ifdef HAVE_LIBSENSORS
                return sensors_get_feature_wrapper (chip->chip_name, number, value);
            #else
                return -1;
            #endif
            break;
        }
        case HDD: {
            #ifdef HAVE_HDDTEMP
                g_assert (number<chip->num_features);
                feature = (t_chipfeature *) g_ptr_array_index (chip->chip_features, number);
                g_assert (feature!=NULL);
                *value = get_hddtemp_value (feature->devicename, suppress);
                if (*value==NO_VALID_HDDTEMP_PROGRAM) {
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
                g_assert (number<chip->num_features);
                feature = (t_chipfeature *) g_ptr_array_index (chip->chip_features, number);
                g_assert (feature!=NULL);
                /* TODO: seperate refresh from get operation! */
                refresh_acpi ((gpointer) feature, NULL);
                *value = feature->raw_value;
                return 0;
            #else
                return -1;
            #endif
            break;
        }
        case GPU: {
            #ifdef HAVE_NVIDIA
                g_assert (number<chip->num_features);
                feature = (t_chipfeature *) g_ptr_array_index (chip->chip_features, number);
                g_assert (feature!=NULL);
                /* TODO: seperate refresh from get operation! */
                refresh_nvidia ((gpointer) feature, NULL);
                *value = feature->raw_value;
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
free_chipfeature (gpointer chipfeature, gpointer data)
{
    t_chipfeature *ptr_chipfeature;
    ptr_chipfeature = (t_chipfeature *) chipfeature;

    g_free (ptr_chipfeature->name);
    g_free (ptr_chipfeature->devicename);
    g_free (ptr_chipfeature->formatted_value);
    g_free (ptr_chipfeature->color);
    g_free (ptr_chipfeature);
}


/* -------------------------------------------------------------------------- */
void
free_chip (gpointer chip, gpointer data)
{
    t_chip *ptr_chip_structure;
    ptr_chip_structure = (t_chip *) chip;

    g_free (ptr_chip_structure->description);
    g_free (ptr_chip_structure->name);
    g_free (ptr_chip_structure->sensorId);

#ifdef HAVE_LIBSENSORS
    if (ptr_chip_structure->type==LMSENSOR) {
        free_lmsensors_chip (chip);
    }
#endif
#ifdef HAVE_ACPI
    if (ptr_chip_structure->type==ACPI) {
        free_acpi_chip (chip);
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
