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

#include <config.h>

#ifdef HAVE_LINUX
#include <sys/utsname.h>
#endif

#include "middlelayer.h"

#ifdef HAVE_LIBSENSORS
    #include "lmsensors.h"
#endif
#ifdef HAVE_HDDTEMP
    #include "hddtemp.h"
#endif
#ifdef HAVE_ACPI
    #include "acpi.h"
#endif

int
initialize_all (GPtrArray **chips)
{
    int res = 0;

    TRACE ("enters initialize_all");

    *chips = g_ptr_array_new();

     #ifdef HAVE_LIBSENSORS
    res += initialize_libsensors (*chips);
    #endif

    #ifdef HAVE_HDDTEMP
    res += initialize_hddtemp (*chips);
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
            g_ptr_array_foreach (c->chip_features, refresh_hddtemp, NULL );
        return;
        }
    #endif

    TRACE ("leaves refresh_chip");
}


void
refresh_all_chips (GPtrArray *chips )
{
    TRACE ("enters refresh_all_chips");

    g_ptr_array_foreach (chips, refresh_chip, NULL );

    TRACE ("leaves refresh_all_chips");
}


void
categorize_sensor_type (t_chipfeature* chipfeature)
{
    TRACE ("enters categorize_sensor_type");

    /* categorize sensor type */
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
         chipfeature->min_value = 2.8;
         chipfeature->max_value = 12.2;
   } else if ( strstr(chipfeature->name, "Fan")!=NULL
      || strstr(chipfeature->name, "fan")!=NULL ) {
         chipfeature->class = SPEED;
         chipfeature->min_value = 1000.0;
         chipfeature->max_value = 3500.0;
   } else {
         chipfeature->class = OTHER;
         chipfeature->min_value = 0.0;
         chipfeature->max_value = 7000.0;
   }

   TRACE ("leaves categorize_sensor_type");
}


int
sensors_get_feature_wrapper (t_chip *chip, int number, double *value)
{
    t_chipfeature *feature;
    /* TRACE ("enters sensors_get_feature_wrapper %d", number); */

    g_assert (chip!=NULL);

    if (chip->type==LMSENSOR ) {
        #ifdef HAVE_LIBSENSORS
            return sensors_get_feature (*(chip->chip_name), number, value);
        #else
            return -1;
        #endif
    }
    if (chip->type==HDD ) {
        #ifdef HAVE_HDDTEMP
            g_assert (number<chip->num_features);
            feature = (t_chipfeature *) g_ptr_array_index (chip->chip_features, number);
            g_assert (feature!=NULL);
            *value = get_hddtemp_value (feature->name);
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
            *value = get_acpi_zone_value (feature->name);
            return 0; /* HERE    I    AM,    I    WANNA    BE    FIXED    */
        #else
            return -1;
        #endif
    }
    return -1;
}
