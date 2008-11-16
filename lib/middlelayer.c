/* $Id$ */
/*
 *      middlelayer.c
 *
 *      Copyright 2006, 2007 Fabian Nowak <timytery@arcor.de>
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

int
initialize_all (GPtrArray **chips, gboolean *suppressmessage)
{
    int res = 0;

    TRACE ("enters initialize_all");

    *chips = g_ptr_array_new();

     #ifdef HAVE_LIBSENSORS
    res += initialize_libsensors (*chips);
    #endif

    #ifdef HAVE_HDDTEMP
    res += initialize_hddtemp (*chips, suppressmessage);
    #endif

    #ifdef HAVE_ACPI
    res += initialize_ACPI (*chips);
    #endif

    TRACE ("leaves initialize_all, chips->len=%d", (*chips)->len);

    return res;
}


void
refresh_chip (gpointer chip, gpointer data)
{
    t_chip *c;

    g_assert (chip!=NULL);

    TRACE ("enters refresh_chip");

    c = (t_chip*) chip;

    #ifdef HAVE_ACPI
        if (c->type==ACPI) {
            g_ptr_array_foreach (c->chip_features, refresh_acpi, NULL );
            return;
        }
    #endif

    #ifdef HAVE_LIBSENSORS
        if (c->type==LMSENSOR) {
            g_ptr_array_foreach (c->chip_features, refresh_lmsensors, NULL );
        return;
        }
    #endif

    #ifdef HAVE_HDDTEMP
        if (c->type==HDD) {
            g_ptr_array_foreach (c->chip_features, refresh_hddtemp, data ); /* note that data is of *t_sensors! */
        return;
        }
    #endif

    TRACE ("leaves refresh_chip");
}


void
refresh_all_chips (GPtrArray *chips, t_sensors *sensors )
{
    TRACE ("enters refresh_all_chips");

    g_ptr_array_foreach (chips, refresh_chip, sensors);

    TRACE ("leaves refresh_all_chips");
}


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


int
sensor_get_value (t_chip *chip, int number, double *value, gboolean *suppressmessage)
{
    t_chipfeature *feature;
    gboolean *suppress = suppressmessage;
    /* TRACE ("enters sensor_get_value %d", number); */

    g_assert (chip!=NULL);

    if (chip->type==LMSENSOR ) {
        #ifdef HAVE_LIBSENSORS
            return sensors_get_feature_wrapper (chip->chip_name, number, value);
        #else
            return -1;
        #endif
    }
    if (chip->type==HDD ) {
        #ifdef HAVE_HDDTEMP
            g_assert (number<chip->num_features);
            feature = (t_chipfeature *) g_ptr_array_index (chip->chip_features, number);
            g_assert (feature!=NULL);
            *value = get_hddtemp_value (feature->devicename, suppress);
            if (*value==ZERO_KELVIN) {
                return NO_VALID_HDDTEMP;
            }
            return 0;
        #else
            return -1;
        #endif
    }
    if (chip->type==ACPI ) {
        #ifdef HAVE_ACPI
            g_assert (number<chip->num_features);
            feature = (t_chipfeature *) g_ptr_array_index (chip->chip_features, number);
            g_assert (feature!=NULL);
            refresh_acpi ((gpointer) feature, NULL);
            *value = feature->raw_value;
            return 0; /* HERE    I    AM,    I    WANNA    BE    FIXED    */
        #else
            return -1;
        #endif
    }
    else {
        feature = NULL;
        return -1;
    }
}


void
free_chipfeature (gpointer chipfeature, gpointer data)
{
    t_chipfeature *cf;
    cf = (t_chipfeature *) chipfeature;

    g_free (cf->name);
    g_free (cf->devicename);
    g_free (cf->formatted_value);
    g_free (cf->color);
}


void
free_chip (gpointer chip, gpointer data)
{
    t_chip *c;
    c = (t_chip *) chip;

    g_free (c->sensorId);
    g_free (c->description);
    #ifdef HAVE_LIBSENSORS
    if (c->type==LMSENSOR) {
        free_lmsensors_chip (chip);
    }
    #endif
    /* g_free (c->chip_name); */   /* is a _copied_ structure of libsensors */
    g_ptr_array_foreach (c->chip_features, free_chipfeature, NULL);
    g_ptr_array_free (c->chip_features, TRUE);
}


void
sensor_interface_cleanup()
{
    #ifdef HAVE_LIBSENSORS
        sensors_cleanup();
    #endif
}
