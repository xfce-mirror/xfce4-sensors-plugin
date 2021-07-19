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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <errno.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <stdio.h>
#include <string.h>

/* Package includes */
#include <lmsensors.h>
#include <middlelayer.h>
#include <types.h>


/* -------------------------------------------------------------------------- */
int
sensors_get_feature_wrapper (const sensors_chip_name *chip_name,
                             int address_chipfeature, double *out_value)
{
    int result;
    #if SENSORS_API_VERSION < 0x400 /* libsensors3 */
        result = sensors_get_feature (*chip_name, address_chipfeature, out_value);
    #else
        result = sensors_get_value (chip_name, address_chipfeature, out_value);
    #endif
    return result;
}


/* -------------------------------------------------------------------------- */
static t_chip*
setup_chip (GPtrArray *chips, const sensors_chip_name *chip_name, int num_sensorchips)
{
    t_chip *chip;

    chip = g_new0 (t_chip, 1);

    g_ptr_array_add (chips, chip);

    chip->chip_name = (sensors_chip_name*) g_malloc (sizeof (sensors_chip_name));
    memcpy ((void*) (chip->chip_name), (void*) chip_name, sizeof (sensors_chip_name));

    #if SENSORS_API_VERSION < 0x400 /* libsensor 3 code */
        chip->sensorId = g_strdup_printf ("%s-%x-%x", chip_name->prefix, chip_name->bus,
                                          chip_name->addr);
    #else
        switch (chip_name->bus.type) {
            case SENSORS_BUS_TYPE_I2C:
            case SENSORS_BUS_TYPE_SPI:
                chip->sensorId = g_strdup_printf ("%s-%x-%x", chip_name->prefix,
                                                  chip_name->bus.nr, chip_name->addr);
                break;
            default:
                chip->sensorId = g_strdup_printf ("%s-%x", chip_name->prefix,
                                                  chip_name->addr);
        }
    #endif

    chip->num_features = 0;
    chip->name = g_strdup (_("LM Sensors"));
    chip->chip_features = g_ptr_array_new ();

    #if SENSORS_API_VERSION < 0x400 /* libsensors3 */
        chip->description = g_strdup (sensors_get_adapter_name (num_sensorchips-1));
    #else
        chip->description = g_strdup (sensors_get_adapter_name (&chip_name->bus));
    #endif

    return chip;
}


/* -------------------------------------------------------------------------- */
#if SENSORS_API_VERSION >= 0x400 /* libsensors4 */
static void
categorize_sensor_type_libsensors4 (t_chipfeature *chip_feature,
                                    const sensors_feature *sensorsfeature,
                                    const sensors_chip_name *chip_name,
                                    int address_chipfeature)
{
    const sensors_subfeature *sub_feature = NULL;
    double sensorFeature;

    switch (sensorsfeature->type) {
        case SENSORS_FEATURE_IN:
            chip_feature->class = VOLTAGE;
            chip_feature->min_value = 1.0;
            chip_feature->max_value = 12.2;

            if ((sub_feature = sensors_get_subfeature (chip_name, sensorsfeature,
                    SENSORS_SUBFEATURE_IN_MIN)) &&
                    !sensors_get_value (chip_name, address_chipfeature, &sensorFeature))
                chip_feature->min_value = sensorFeature;

            if ((sub_feature = sensors_get_subfeature (chip_name, sensorsfeature,
                    SENSORS_SUBFEATURE_IN_MAX)) &&
                    !sensors_get_value (chip_name, address_chipfeature, &sensorFeature))
                chip_feature->max_value = sensorFeature;

            break;

        case SENSORS_FEATURE_FAN:
            chip_feature->class = SPEED;
            chip_feature->min_value = 1000.0;
            chip_feature->max_value = 3500.0;

            if ((sub_feature = sensors_get_subfeature (chip_name, sensorsfeature,
                    SENSORS_SUBFEATURE_FAN_MIN)) &&
                    !sensors_get_value (chip_name, address_chipfeature, &sensorFeature))
                chip_feature->min_value = sensorFeature;

            break;

        case SENSORS_FEATURE_TEMP:
            chip_feature->class = TEMPERATURE;
            chip_feature->min_value = 0.0;
            chip_feature->max_value = 80.0;

            if ((sub_feature = sensors_get_subfeature (chip_name, sensorsfeature,
                    SENSORS_SUBFEATURE_TEMP_MIN)) &&
                    !sensors_get_value (chip_name, address_chipfeature, &sensorFeature))
                chip_feature->min_value = sensorFeature;

            if (((sub_feature = sensors_get_subfeature (chip_name, sensorsfeature,
                    SENSORS_SUBFEATURE_TEMP_MAX)) ||
                    (sub_feature = sensors_get_subfeature (chip_name, sensorsfeature,
                    SENSORS_SUBFEATURE_TEMP_CRIT))) &&
                    !sensors_get_value (chip_name, address_chipfeature, &sensorFeature))
                chip_feature->max_value = sensorFeature;
            break;

        case SENSORS_FEATURE_POWER:
            chip_feature->class = POWER;
            chip_feature->min_value = 0.0;
            chip_feature->max_value = 120.0;

            if ((sub_feature = sensors_get_subfeature (chip_name, sensorsfeature,
                    SENSORS_SUBFEATURE_POWER_MAX)) &&
                    !sensors_get_value (chip_name, address_chipfeature, &sensorFeature))
                chip_feature->max_value = sensorFeature;

            break;

        case SENSORS_FEATURE_ENERGY:
            chip_feature->class = ENERGY;
            chip_feature->min_value = 0.0;
            chip_feature->max_value = 120.0;
            break;

        case SENSORS_FEATURE_CURR:
            chip_feature->class = CURRENT;
            chip_feature->min_value = 0.0;
            chip_feature->max_value = 100.0;

            if ((sub_feature = sensors_get_subfeature (chip_name, sensorsfeature,
                    SENSORS_SUBFEATURE_CURR_MIN)) &&
                    !sensors_get_value (chip_name, address_chipfeature, &sensorFeature))
                chip_feature->min_value = sensorFeature;

            if ((sub_feature = sensors_get_subfeature (chip_name, sensorsfeature,
                    SENSORS_SUBFEATURE_CURR_MAX)) &&
                    !sensors_get_value (chip_name, address_chipfeature, &sensorFeature))
                chip_feature->max_value = sensorFeature;

            break;

        case SENSORS_FEATURE_VID:
            chip_feature->class = VOLTAGE;
            chip_feature->min_value = 1.0;
            chip_feature->max_value = 3.5;
            break;

        case SENSORS_FEATURE_BEEP_ENABLE:
            chip_feature->class = STATE;
            chip_feature->min_value = 1.0;
            chip_feature->max_value = 3.5;
            break;

        default: /* UNKNOWN */
            chip_feature->class = OTHER;
            chip_feature->min_value = 0.0;
            chip_feature->max_value = 7000.0;
    }
}
#endif


/* -------------------------------------------------------------------------- */
static void
setup_chipfeature_common (t_chipfeature *feature, int address_chipfeature,
                          double val_sensor_feature)
{
    g_free (feature->color_orNull);
    feature->color_orNull = g_strdup ("#00B000");
    feature->valid = TRUE;

    feature->raw_value = val_sensor_feature;
    feature->address = address_chipfeature;
    feature->show = FALSE;
}


#if SENSORS_API_VERSION < 0x400 /* libsensors3 */
/* -------------------------------------------------------------------------- */
void
setup_chipfeature (t_chipfeature *feature, int address_chipfeature,
                   double val_sensor_feature)
{
    setup_chipfeature_common (feature, address_chipfeature, val_sensor_feature);

    categorize_sensor_type (feature);
}
#else
/* -------------------------------------------------------------------------- */
static void
setup_chipfeature_libsensors4 (t_chipfeature *chip_feature,
                               const sensors_feature *feature,
                               int address_chipfeature,
                               double val_sensor_feature,
                               const sensors_chip_name *name)
{
    setup_chipfeature_common (chip_feature, address_chipfeature, val_sensor_feature);

    categorize_sensor_type_libsensors4 (chip_feature, feature, name, address_chipfeature);
}
#endif


#if SENSORS_API_VERSION < 0x400 /* libsensors3 */
/* -------------------------------------------------------------------------- */
static t_chipfeature *
find_chipfeature (const sensors_chip_name *name, t_chip *chip,
                  int address_chipfeature)
{
    int res;
    double sensorFeature;
    t_chipfeature *chipfeature;

    chipfeature = g_new0 (t_chipfeature, 1);

    if (sensors_get_ignored (*(name), address_chipfeature)==1) {
        g_free (chipfeature->name); /*  ?  */
        res = sensors_get_label (*(name), address_chipfeature, &chipfeature->name);

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
static t_chipfeature *
find_chipfeature (const sensors_chip_name *name, t_chip *chip,
                  const sensors_feature *feature)
{
    const sensors_subfeature *sub_feature = NULL;
    int res, number = -1;
    double sensorFeature;

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
        t_chipfeature *chip_feature = g_new0 (t_chipfeature, 1);

        chip_feature->name = sensors_get_label (name, feature);

        if (!chip_feature->name && feature->name)
            chip_feature->name = g_strdup(feature->name);

        if (chip_feature->name)
        {
            res = sensors_get_value (name, number, &sensorFeature);
            if (res==0)
            {
                setup_chipfeature_libsensors4 (chip_feature, feature, number,
                                               sensorFeature, name);
                chip->num_features++;
                return chip_feature;
            }
        }

        g_free(chip_feature->name);
        g_free(chip_feature);
    }

    return NULL;
}
#endif


/* -------------------------------------------------------------------------- */
int
initialize_libsensors (GPtrArray *chips)
{
    int sensorsInit, nr1, num_sensorchips;
    t_chip *chip;
    t_chipfeature *chip_feature;
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
            chip = setup_chip (chips, detected_chip, num_sensorchips);

            nr1 = 0;
            nr2 = 0;
            /* iterate over chip features, i.e. id, cpu temp, mb temp... */
            /* numchips = get_number_chip_features (detected_chip); */
            sfd = sensors_get_all_features (*detected_chip, &nr1, &nr2);
            while (sfd != NULL)
            {
                chip_feature = find_chipfeature (detected_chip, chip, sfd->number);
                if (chip_feature!=NULL) {
                    g_ptr_array_add (chip->chip_features, chip_feature);
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
        g_printf (_("Error: Could not connect to sensors!"));
        /* FIXME: better popup window? write to special logfile? */
        return -2;
    }

    num_sensorchips = 0;
    detected_chip = sensors_get_detected_chips (NULL, &num_sensorchips);
    /* iterate over chips on mainboard */
    while (detected_chip!=NULL)
    {
        chip = setup_chip (chips, detected_chip, num_sensorchips);

        nr1 = 0;
        /* iterate over chip features, i.e. id, cpu temp, mb temp... */
        /* numchips = get_number_chip_features (detected_chip); */
        sfd = sensors_get_features (detected_chip, &nr1);
        while (sfd != NULL)
        {
            chip_feature = find_chipfeature (detected_chip, chip, sfd);
            if (chip_feature!=NULL) {
                g_ptr_array_add (chip->chip_features, chip_feature);
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
refresh_lmsensors (gpointer chip_feature, gpointer unused)
{
    g_assert (chip_feature != NULL);
}


/* -------------------------------------------------------------------------- */
void
free_lmsensors_chip (gpointer ptr_chip)
{
    #if SENSORS_API_VERSION < 0x400
    t_chip *chip = (t_chip*) ptr_chip;
    if (chip->chip_name->busname)
        g_free (chip->chip_name->busname);
    #endif
}
