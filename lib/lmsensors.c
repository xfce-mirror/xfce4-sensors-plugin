/* File: lmsensors.c
 *
 * Copyright 2007-2017 Fabian Nowak (timystery@arcor.de)
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/* Note for programmers and editors: Try to use 4 spaces instead of Tab! */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/* Package includes */
#include <lmsensors.h>
#include <middlelayer.h>
#include <types.h>

/* Gtk/Glib includes */
#include <glib.h>
#include <glib/gprintf.h>

/* Global includes */
#include <errno.h>
#include <stdio.h>
#include <string.h>

/* internal forward declaration so that GCC 4.4 does not complain */
t_chip * setup_chip (GPtrArray *chips, const sensors_chip_name *c_ptr_sensors_chip_name, int num_sensorchips);
void categorize_sensor_type_libsensors4 (t_chipfeature *ptr_chipfeature, const sensors_feature *c_ptr_sensors_feature, const sensors_chip_name *c_ptr_sensors_chip_name, int address_chipfeature);
void setup_chipfeature_common (t_chipfeature *ptr_chipfeature, int address_chipfeature, double val_sensor_feature);
void setup_chipfeature_libsensors4 (t_chipfeature *ptr_chipfeature, const sensors_feature *feature, int address_chipfeature, double val_sensor_feature, const sensors_chip_name *name);
t_chipfeature * find_chipfeature (const sensors_chip_name *name, t_chip *chip, const sensors_feature *feature);



/* -------------------------------------------------------------------------- */
int
sensors_get_feature_wrapper (const sensors_chip_name *c_ptr_sensors_chip_name,
                             int address_chipfeature, double *value)
{
    int result;
    #if SENSORS_API_VERSION < 0x400 /* libsensors3 */
        result = sensors_get_feature (*c_ptr_sensors_chip_name, address_chipfeature, value);
    #else
        result = sensors_get_value (c_ptr_sensors_chip_name, address_chipfeature, value);
    #endif
    return result;
}


/* -------------------------------------------------------------------------- */
t_chip *
setup_chip (GPtrArray *chips, const sensors_chip_name *c_ptr_sensors_chip_name,
            int num_sensorchips)
{
    t_chip* chip;

    chip = g_new0 (t_chip, 1);

    g_ptr_array_add (chips, chip);

    chip->chip_name = (sensors_chip_name *) g_malloc (sizeof(sensors_chip_name));
    memcpy ( (void *) (chip->chip_name), (void *) c_ptr_sensors_chip_name, sizeof(sensors_chip_name) );

    #if SENSORS_API_VERSION < 0x400 /* libsensor 3 code */
        chip->sensorId = g_strdup_printf ("%s-%x-%x", c_ptr_sensors_chip_name->prefix, c_ptr_sensors_chip_name->bus,
                                            c_ptr_sensors_chip_name->addr);
    #else
        switch (c_ptr_sensors_chip_name->bus.type) {
            case SENSORS_BUS_TYPE_I2C:
            case SENSORS_BUS_TYPE_SPI:
                chip->sensorId = g_strdup_printf ("%s-%x-%x", c_ptr_sensors_chip_name->prefix,
                                                    c_ptr_sensors_chip_name->bus.nr, c_ptr_sensors_chip_name->addr);
                break;
            default:
                chip->sensorId = g_strdup_printf ("%s-%x", c_ptr_sensors_chip_name->prefix,
                                                    c_ptr_sensors_chip_name->addr);
        }
    #endif

    chip->num_features=0;
    chip->name = g_strdup(_("LM Sensors"));
    chip->chip_features = g_ptr_array_new();

    #if SENSORS_API_VERSION < 0x400 /* libsensors3 */
        chip->description = g_strdup (
                                sensors_get_adapter_name (num_sensorchips-1));
    #else
        chip->description = g_strdup (
                                sensors_get_adapter_name (&c_ptr_sensors_chip_name->bus));
    #endif

    return chip;
}


/* -------------------------------------------------------------------------- */
#if SENSORS_API_VERSION >= 0x400 /* libsensors4 */
void
categorize_sensor_type_libsensors4 (t_chipfeature *ptr_chipfeature,
                                    const sensors_feature *c_ptr_sensors_feature,
                                    const sensors_chip_name *c_ptr_sensors_chip_name,
                                    int address_chipfeature)
{
    const sensors_subfeature *sub_feature = NULL;
    double sensorFeature;

    switch (c_ptr_sensors_feature->type) {
        case SENSORS_FEATURE_IN:
            ptr_chipfeature->class = VOLTAGE;
            ptr_chipfeature->min_value = 1.0;
            ptr_chipfeature->max_value = 12.2;

            if ((sub_feature = sensors_get_subfeature (c_ptr_sensors_chip_name, c_ptr_sensors_feature,
                    SENSORS_SUBFEATURE_IN_MIN)) &&
                    !sensors_get_value (c_ptr_sensors_chip_name, address_chipfeature, &sensorFeature))
                ptr_chipfeature->min_value = sensorFeature;

            if ((sub_feature = sensors_get_subfeature (c_ptr_sensors_chip_name, c_ptr_sensors_feature,
                    SENSORS_SUBFEATURE_IN_MAX)) &&
                    !sensors_get_value (c_ptr_sensors_chip_name, address_chipfeature, &sensorFeature))
                ptr_chipfeature->max_value = sensorFeature;

            break;

        case SENSORS_FEATURE_FAN:
            ptr_chipfeature->class = SPEED;
            ptr_chipfeature->min_value = 1000.0;
            ptr_chipfeature->max_value = 3500.0;

            if ((sub_feature = sensors_get_subfeature (c_ptr_sensors_chip_name, c_ptr_sensors_feature,
                    SENSORS_SUBFEATURE_FAN_MIN)) &&
                    !sensors_get_value (c_ptr_sensors_chip_name, address_chipfeature, &sensorFeature))
                ptr_chipfeature->min_value = sensorFeature;

            break;

        case SENSORS_FEATURE_TEMP:
            ptr_chipfeature->class = TEMPERATURE;
            ptr_chipfeature->min_value = 0.0;
            ptr_chipfeature->max_value = 80.0;

            if ((sub_feature = sensors_get_subfeature (c_ptr_sensors_chip_name, c_ptr_sensors_feature,
                    SENSORS_SUBFEATURE_TEMP_MIN)) &&
                    !sensors_get_value (c_ptr_sensors_chip_name, address_chipfeature, &sensorFeature))
                ptr_chipfeature->min_value = sensorFeature;

            if (((sub_feature = sensors_get_subfeature (c_ptr_sensors_chip_name, c_ptr_sensors_feature,
                    SENSORS_SUBFEATURE_TEMP_MAX)) ||
                    (sub_feature = sensors_get_subfeature (c_ptr_sensors_chip_name, c_ptr_sensors_feature,
                    SENSORS_SUBFEATURE_TEMP_CRIT))) &&
                    !sensors_get_value (c_ptr_sensors_chip_name, address_chipfeature, &sensorFeature))
                ptr_chipfeature->max_value = sensorFeature;
            break;

        case SENSORS_FEATURE_POWER:
            ptr_chipfeature->class = POWER;
            ptr_chipfeature->min_value = 0.0;
            ptr_chipfeature->max_value = 120.0;

            if ((sub_feature = sensors_get_subfeature (c_ptr_sensors_chip_name, c_ptr_sensors_feature,
                    SENSORS_SUBFEATURE_POWER_MAX)) &&
                    !sensors_get_value (c_ptr_sensors_chip_name, address_chipfeature, &sensorFeature))
                ptr_chipfeature->max_value = sensorFeature;

            break;

        case SENSORS_FEATURE_ENERGY:
	    ptr_chipfeature->class = ENERGY;
            ptr_chipfeature->min_value = 0.0;
            ptr_chipfeature->max_value = 120.0;
	    break;

        case SENSORS_FEATURE_CURR:
	    ptr_chipfeature->class = CURRENT;
            ptr_chipfeature->min_value = 0.0;
            ptr_chipfeature->max_value = 100.0;

            if ((sub_feature = sensors_get_subfeature (c_ptr_sensors_chip_name, c_ptr_sensors_feature,
                    SENSORS_SUBFEATURE_CURR_MIN)) &&
                    !sensors_get_value (c_ptr_sensors_chip_name, address_chipfeature, &sensorFeature))
                ptr_chipfeature->min_value = sensorFeature;

            if ((sub_feature = sensors_get_subfeature (c_ptr_sensors_chip_name, c_ptr_sensors_feature,
                    SENSORS_SUBFEATURE_CURR_MAX)) &&
                    !sensors_get_value (c_ptr_sensors_chip_name, address_chipfeature, &sensorFeature))
                ptr_chipfeature->max_value = sensorFeature;

	    break;

        case SENSORS_FEATURE_VID:
            ptr_chipfeature->class = VOLTAGE;
            ptr_chipfeature->min_value = 1.0;
            ptr_chipfeature->max_value = 3.5;
            break;

        case SENSORS_FEATURE_BEEP_ENABLE:
            ptr_chipfeature->class = STATE;
            ptr_chipfeature->min_value = 1.0;
            ptr_chipfeature->max_value = 3.5;
            break;

        default: /* UNKNOWN */
            ptr_chipfeature->class = OTHER;
            ptr_chipfeature->min_value = 0.0;
            ptr_chipfeature->max_value = 7000.0;
    }
}
#endif


/* -------------------------------------------------------------------------- */
void
setup_chipfeature_common (t_chipfeature *ptr_chipfeature, int address_chipfeature,
                          double val_sensor_feature)
{
    g_free (ptr_chipfeature->color);
    ptr_chipfeature->color = g_strdup("#00B000");
    ptr_chipfeature->valid = TRUE;

    ptr_chipfeature->raw_value = val_sensor_feature;
    ptr_chipfeature->address = address_chipfeature;
    ptr_chipfeature->show = FALSE;
}


#if SENSORS_API_VERSION < 0x400 /* libsensors3 */
/* -------------------------------------------------------------------------- */
void
setup_chipfeature (t_chipfeature *chipfeature, int address_chipfeature,
                   double val_sensor_feature)
{
    setup_chipfeature_common (chipfeature, address_chipfeature, val_sensor_feature);

    categorize_sensor_type (chipfeature);
}
#else
/* -------------------------------------------------------------------------- */
void
setup_chipfeature_libsensors4 (t_chipfeature *ptr_chipfeature,
                               const sensors_feature *feature,
                               int address_chipfeature,
                               double val_sensor_feature,
                               const sensors_chip_name *name)
{

    setup_chipfeature_common (ptr_chipfeature, address_chipfeature, val_sensor_feature);

    categorize_sensor_type_libsensors4 (ptr_chipfeature, feature, name, address_chipfeature);
}
#endif


#if SENSORS_API_VERSION < 0x400 /* libsensors3 */
/* -------------------------------------------------------------------------- */
t_chipfeature *
find_chipfeature (const sensors_chip_name *name, t_chip *chip,
                  int address_chipfeature)
{
    int res;
    double sensorFeature;
    t_chipfeature *chipfeature;

    chipfeature = g_new0 (t_chipfeature, 1);

    if (sensors_get_ignored (*(name), address_chipfeature)==1) {
        g_free (chipfeature->name); /*  ?  */
        res = sensors_get_label (*(name), address_chipfeature, &(chipfeature->name));

        if (res==0) {
            res = sensors_get_feature (*(name), address_chipfeature, &sensorFeature);

            if (res==0) {
                setup_chipfeature (chipfeature, address_chipfeature, sensorFeature);
                chip->num_features++;
                return chipfeature;
            }
        }
    }

    g_free (chipfeature);
    return NULL;
}
#else
/* -------------------------------------------------------------------------- */
t_chipfeature *
find_chipfeature (const sensors_chip_name *name, t_chip *chip,
                  const sensors_feature *feature)
{
    const sensors_subfeature *sub_feature = NULL;
    int res, number = -1;
    double sensorFeature;
    t_chipfeature *chipfeature;

    switch (feature->type) {
        case SENSORS_FEATURE_IN:
            sub_feature = sensors_get_subfeature (name, feature,
                                                  SENSORS_SUBFEATURE_IN_INPUT);
            break;
        case SENSORS_FEATURE_FAN:
            sub_feature = sensors_get_subfeature (name, feature,
                                                 SENSORS_SUBFEATURE_FAN_INPUT);
            break;
        case SENSORS_FEATURE_TEMP:
            sub_feature = sensors_get_subfeature (name, feature,
                                                 SENSORS_SUBFEATURE_TEMP_INPUT);
            break;
        case SENSORS_FEATURE_VID:
            sub_feature = sensors_get_subfeature (name, feature,
                                                 SENSORS_SUBFEATURE_VID);
            break;
        case SENSORS_FEATURE_BEEP_ENABLE:
            sub_feature = sensors_get_subfeature (name, feature,
                                                 SENSORS_SUBFEATURE_BEEP_ENABLE);
            break;
        case SENSORS_FEATURE_POWER:
            sub_feature = sensors_get_subfeature (name, feature,
                                                 SENSORS_SUBFEATURE_POWER_INPUT);
            break;
        case SENSORS_FEATURE_ENERGY:
            sub_feature = sensors_get_subfeature (name, feature,
                                                 SENSORS_SUBFEATURE_ENERGY_INPUT);
            break;
        case SENSORS_FEATURE_CURR:
            sub_feature = sensors_get_subfeature (name, feature,
                                                 SENSORS_SUBFEATURE_CURR_INPUT);
            break;
        default:
            sub_feature = sensors_get_subfeature (name, feature,
                                                  SENSORS_SUBFEATURE_UNKNOWN);
    }
    if (sub_feature)
        number = sub_feature->number;

    if (number!=-1)
    {
        chipfeature = g_new0 (t_chipfeature, 1);

        chipfeature->name = sensors_get_label (name, feature);

        if (!chipfeature->name && feature->name)
            chipfeature->name = g_strdup(feature->name);

        if (chipfeature->name)
        {
            res = sensors_get_value (name, number, &sensorFeature);
            if (res==0)
            {
                setup_chipfeature_libsensors4 (chipfeature, feature, number,
                                               sensorFeature, name);
                chip->num_features++;
                return chipfeature;
            }
        }

        g_free(chipfeature->name);
        g_free(chipfeature);
    }

    return NULL;
}
#endif


/* -------------------------------------------------------------------------- */
int
initialize_libsensors (GPtrArray *arr_ptr_chips)
{
    int sensorsInit, nr1, num_sensorchips; /*    , numchips;  */
    t_chip *chip;
    t_chipfeature *chipfeature; /* , *furtherchipfeature; */
    const sensors_chip_name *detected_chip;
#if SENSORS_API_VERSION < 0x400 /* libsensors3 */
    FILE *file;
    const sensors_feature_data *sfd;
        int nr2;

    errno = 0;
    file = fopen("/etc/sensors.conf", "r");

    if (errno != ENOENT) /* the file actually exists */
    {
        sensorsInit = sensors_init (file);
        if (sensorsInit != 0)
        {
            g_printf(_("Error: Could not connect to sensors!"));
            /* FIXME: better popup window? write to special logfile? */
            fclose (file);
            return -2;
        }

        num_sensorchips = 0;
        detected_chip = sensors_get_detected_chips ( &num_sensorchips);

        /* iterate over chips on mainboard */
        while (detected_chip!=NULL)
        {
            chip = setup_chip (arr_ptr_chips, detected_chip, num_sensorchips);

            nr1 = 0;
            nr2 = 0;
            /* iterate over chip features, i.e. id, cpu temp, mb temp... */
            /* numchips = get_number_chip_features (detected_chip); */
            sfd = sensors_get_all_features (*detected_chip, &nr1, &nr2);
            while (sfd != NULL)
            {
                chipfeature = find_chipfeature (detected_chip, chip, sfd->number);
                if (chipfeature!=NULL) {
                    g_ptr_array_add (chip->chip_features, chipfeature);
                }
                sfd = sensors_get_all_features (*detected_chip, &nr1, &nr2);
            }

            detected_chip = sensors_get_detected_chips (&num_sensorchips);
        } /* end while sensor chipNames */

        fclose (file);
        return 1;
    }
    else {
        fclose (file);
        return -1;
    }
#else
    const sensors_feature *sfd;

    sensorsInit = sensors_init (NULL);
    if (sensorsInit != 0)
    {
        g_printf(_("Error: Could not connect to sensors!"));
        /* FIXME: better popup window? write to special logfile? */
        return -2;
    }

    num_sensorchips = 0;
    detected_chip = sensors_get_detected_chips (NULL, &num_sensorchips);
    /* iterate over chips on mainboard */
    while (detected_chip!=NULL)
    {
        chip = setup_chip (arr_ptr_chips, detected_chip, num_sensorchips);

        nr1 = 0;
        /* iterate over chip features, i.e. id, cpu temp, mb temp... */
        /* numchips = get_number_chip_features (detected_chip); */
        sfd = sensors_get_features (detected_chip, &nr1);
        while (sfd != NULL)
        {
            chipfeature = find_chipfeature (detected_chip, chip, sfd);
            if (chipfeature!=NULL) {
                g_ptr_array_add (chip->chip_features, chipfeature);
            }
            sfd = sensors_get_features (detected_chip, &nr1);
        }

        detected_chip = sensors_get_detected_chips (NULL, &num_sensorchips);
    } /* end while sensor chipNames */

    return 1;
#endif
}


/* -------------------------------------------------------------------------- */
void
refresh_lmsensors (gpointer ptr_chip_feature, gpointer ptr_unused)
{
    g_assert(ptr_chip_feature!=NULL);
}


/* -------------------------------------------------------------------------- */
void
free_lmsensors_chip (gpointer ptr_chip)
{
    #if SENSORS_API_VERSION < 0x400
    t_chip *ptr_chip_local;
    ptr_chip_local = (t_chip *) ptr_chip;
    if (ptr_chip_local->chip_name->busname)
        g_free (ptr_chip_local->chip_name->busname);
    #endif
}
