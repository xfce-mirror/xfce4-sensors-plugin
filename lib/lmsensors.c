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

#if SENSORS_API_VERSION < 0x400
#error "Unsupported version of lm_sensors"
#endif


/* -------------------------------------------------------------------------- */
static t_chip*
setup_chip (GPtrArray *chips, const sensors_chip_name *chip_name)
{
    t_chip *chip = g_new0 (t_chip, 1);

    g_ptr_array_add (chips, chip);

    chip->chip_name = (sensors_chip_name*) g_malloc (sizeof (sensors_chip_name));
    memcpy ((void*) chip->chip_name, (void*) chip_name, sizeof (sensors_chip_name));

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

    chip->num_features = 0;
    chip->name = g_strdup (_("LM Sensors"));
    chip->chip_features = g_ptr_array_new ();
    chip->description = g_strdup (sensors_get_adapter_name (&chip_name->bus));

    return chip;
}


/* -------------------------------------------------------------------------- */
static void
categorize_sensor_type_libsensors (t_chipfeature *chip_feature,
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


static void
setup_chipfeature_libsensors (t_chipfeature *chip_feature,
                              const sensors_feature *feature,
                              int address_chipfeature,
                              double val_sensor_feature,
                              const sensors_chip_name *name)
{
    setup_chipfeature_common (chip_feature, address_chipfeature, val_sensor_feature);
    categorize_sensor_type_libsensors (chip_feature, feature, name, address_chipfeature);
}


/* -------------------------------------------------------------------------- */
static t_chipfeature *
find_chipfeature (const sensors_chip_name *name, t_chip *chip, const sensors_feature *feature)
{
    const sensors_subfeature *sub_feature = NULL;
    int number = -1;
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
            if (sensors_get_value (name, number, &sensorFeature) == 0)
            {
                setup_chipfeature_libsensors (chip_feature, feature, number, sensorFeature, name);
                chip->num_features++;
                return chip_feature;
            }
        }

        g_free(chip_feature->name);
        g_free(chip_feature);
    }

    return NULL;
}


/* -------------------------------------------------------------------------- */
int
initialize_libsensors (GPtrArray *chips)
{
    int sensorsInit, num_sensorchips;
    const sensors_chip_name *detected_chip;

    sensorsInit = sensors_init (NULL);
    if (sensorsInit != 0)
    {
        g_printf (_("Error: Could not connect to sensors!"));
        /* FIXME: better popup window? write to special logfile? */
        return -2;
    }

    /* iterate over chips on mainboard */
    num_sensorchips = 0;
    detected_chip = sensors_get_detected_chips (NULL, &num_sensorchips);
    while (detected_chip)
    {
        t_chip *chip = setup_chip (chips, detected_chip);
        int nr1 = 0;
        const sensors_feature *sf;

        /* iterate over chip features, i.e. id, cpu temp, mb temp... */
        sf = sensors_get_features (detected_chip, &nr1);
        while (sf != NULL)
        {
            t_chipfeature *chip_feature = find_chipfeature (detected_chip, chip, sf);
            if (chip_feature) {
                g_ptr_array_add (chip->chip_features, chip_feature);
            }
            sf = sensors_get_features (detected_chip, &nr1);
        }

        detected_chip = sensors_get_detected_chips (NULL, &num_sensorchips);
    }

    return 1;
}


/* -------------------------------------------------------------------------- */
void
refresh_lmsensors (gpointer chip_feature, gpointer unused)
{
    g_assert (chip_feature != NULL);
}


/* -------------------------------------------------------------------------- */
void
free_lmsensors_chip (gpointer ptr_chip) {}
