/* lmsensors.cc
 * Part of xfce4-sensors-plugin
 *
 * Copyright (c) 2007-2017 Fabian Nowak <timystery@arcor.de>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/* The fixes file has to be included before any other #include directives */
#include "xfce4++/util/fixes.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <errno.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <stdio.h>
#include <string.h>
#include "xfce4++/util.h"

/* Package includes */
#include <lmsensors.h>
#include <middlelayer.h>
#include <types.h>

#if SENSORS_API_VERSION < 0x400
#error "Unsupported version of lm_sensors"
#endif


/* -------------------------------------------------------------------------- */
static Ptr<t_chip>
setup_chip (std::vector<Ptr<t_chip>> &chips, const sensors_chip_name *chip_name)
{
    auto chip = xfce4::make<t_chip>();

    chips.push_back(chip);

    chip->chip_name = (sensors_chip_name*) g_malloc (sizeof (sensors_chip_name));
    *chip->chip_name = *chip_name;

    switch (chip_name->bus.type) {
        case SENSORS_BUS_TYPE_I2C:
        case SENSORS_BUS_TYPE_SPI:
            chip->sensorId = xfce4::sprintf ("%s-%x-%x", chip_name->prefix, chip_name->bus.nr, chip_name->addr);
            break;
        default:
            chip->sensorId = xfce4::sprintf ("%s-%x", chip_name->prefix, chip_name->addr);
    }

    chip->name = _("LM Sensors");
    chip->description = sensors_get_adapter_name (&chip_name->bus);

    return chip;
}


/* -------------------------------------------------------------------------- */
static void
categorize_sensor_type_libsensors (const Ptr<t_chipfeature> &chip_feature,
                                   const sensors_feature *sensorsfeature,
                                   const sensors_chip_name *chip_name,
                                   int address_chipfeature)
{
    double sensorFeature;

    switch (sensorsfeature->type) {
        case SENSORS_FEATURE_IN:
            chip_feature->cls = VOLTAGE;
            chip_feature->min_value = 1.0;
            chip_feature->max_value = 12.2;

            if (sensors_get_subfeature (chip_name, sensorsfeature,
                    SENSORS_SUBFEATURE_IN_MIN) &&
                    !sensors_get_value (chip_name, address_chipfeature, &sensorFeature))
                chip_feature->min_value = sensorFeature;

            if (sensors_get_subfeature (chip_name, sensorsfeature,
                    SENSORS_SUBFEATURE_IN_MAX) &&
                    !sensors_get_value (chip_name, address_chipfeature, &sensorFeature))
                chip_feature->max_value = sensorFeature;

            break;

        case SENSORS_FEATURE_FAN:
            chip_feature->cls = SPEED;
            chip_feature->min_value = 1000.0;
            chip_feature->max_value = 3500.0;

            if (sensors_get_subfeature (chip_name, sensorsfeature,
                    SENSORS_SUBFEATURE_FAN_MIN) &&
                    !sensors_get_value (chip_name, address_chipfeature, &sensorFeature))
                chip_feature->min_value = sensorFeature;

            break;

        case SENSORS_FEATURE_TEMP:
            chip_feature->cls = TEMPERATURE;
            chip_feature->min_value = 0.0;
            chip_feature->max_value = 80.0;

            if (sensors_get_subfeature (chip_name, sensorsfeature,
                    SENSORS_SUBFEATURE_TEMP_MIN) &&
                    !sensors_get_value (chip_name, address_chipfeature, &sensorFeature))
                chip_feature->min_value = sensorFeature;

            if ((sensors_get_subfeature (chip_name, sensorsfeature,
                    SENSORS_SUBFEATURE_TEMP_MAX) ||
                    sensors_get_subfeature (chip_name, sensorsfeature,
                    SENSORS_SUBFEATURE_TEMP_CRIT)) &&
                    !sensors_get_value (chip_name, address_chipfeature, &sensorFeature))
                chip_feature->max_value = sensorFeature;
            break;

        case SENSORS_FEATURE_POWER:
            chip_feature->cls = POWER;
            chip_feature->min_value = 0.0;
            chip_feature->max_value = 120.0;

            if (sensors_get_subfeature (chip_name, sensorsfeature,
                    SENSORS_SUBFEATURE_POWER_MAX) &&
                    !sensors_get_value (chip_name, address_chipfeature, &sensorFeature))
                chip_feature->max_value = sensorFeature;

            break;

        case SENSORS_FEATURE_ENERGY:
            chip_feature->cls = ENERGY;
            chip_feature->min_value = 0.0;
            chip_feature->max_value = 120.0;
            break;

        case SENSORS_FEATURE_CURR:
            chip_feature->cls = CURRENT;
            chip_feature->min_value = 0.0;
            chip_feature->max_value = 100.0;

            if (sensors_get_subfeature (chip_name, sensorsfeature,
                    SENSORS_SUBFEATURE_CURR_MIN) &&
                    !sensors_get_value (chip_name, address_chipfeature, &sensorFeature))
                chip_feature->min_value = sensorFeature;

            if (sensors_get_subfeature (chip_name, sensorsfeature,
                    SENSORS_SUBFEATURE_CURR_MAX) &&
                    !sensors_get_value (chip_name, address_chipfeature, &sensorFeature))
                chip_feature->max_value = sensorFeature;

            break;

        case SENSORS_FEATURE_VID:
            chip_feature->cls = VOLTAGE;
            chip_feature->min_value = 1.0;
            chip_feature->max_value = 3.5;
            break;

        case SENSORS_FEATURE_BEEP_ENABLE:
            chip_feature->cls = STATE;
            chip_feature->min_value = 1.0;
            chip_feature->max_value = 3.5;
            break;

        default: /* UNKNOWN */
            chip_feature->cls = OTHER;
            chip_feature->min_value = 0.0;
            chip_feature->max_value = 7000.0;
    }
}


/* -------------------------------------------------------------------------- */
static void
setup_chipfeature_common (const Ptr<t_chipfeature> &feature, int address_chipfeature,
                          double val_sensor_feature)
{
    feature->color_orEmpty = "#00B000";
    feature->valid = true;

    feature->raw_value = val_sensor_feature;
    feature->address = address_chipfeature;
    feature->show = false;
}


static void
setup_chipfeature_libsensors (const Ptr<t_chipfeature> &chip_feature,
                              const sensors_feature *feature,
                              int address_chipfeature,
                              double val_sensor_feature,
                              const sensors_chip_name *name)
{
    setup_chipfeature_common (chip_feature, address_chipfeature, val_sensor_feature);
    categorize_sensor_type_libsensors (chip_feature, feature, name, address_chipfeature);
}


/* -------------------------------------------------------------------------- */
static Ptr0<t_chipfeature>
find_chipfeature (const sensors_chip_name *name, const Ptr<t_chip> &chip, const sensors_feature *feature)
{
    const sensors_subfeature *sub_feature = NULL;
    int number = -1;

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

    if (number != -1)
    {
        auto chip_feature = xfce4::make<t_chipfeature>();

        char *label = sensors_get_label (name, feature);
        if (label)
        {
            chip_feature->name = label;
            free (label);
            label = NULL;
        }

        if (chip_feature->name.empty() && feature->name)
            chip_feature->name = feature->name;

        if (!chip_feature->name.empty())
        {
            double sensorFeature;
            if (sensors_get_value (name, number, &sensorFeature) == 0)
            {
                setup_chipfeature_libsensors (chip_feature, feature, number, sensorFeature, name);
                return chip_feature;
            }
        }
    }

    return nullptr;
}


/* -------------------------------------------------------------------------- */
int
initialize_libsensors (std::vector<Ptr<t_chip>> &chips)
{
    if (sensors_init (NULL) != 0)
    {
        g_printf (_("Error: Could not connect to sensors!"));
        /* FIXME: better popup window? write to special logfile? */
        sensors_cleanup ();
        return -2;
    }

    /* iterate over chips on mainboard */
    int num_sensorchips = 0;
    const sensors_chip_name *detected_chip = sensors_get_detected_chips (NULL, &num_sensorchips);
    while (detected_chip)
    {
        auto chip = setup_chip (chips, detected_chip);
        int nr1 = 0;
        const sensors_feature *sf;

        /* iterate over chip features, i.e. id, cpu temp, mb temp... */
        sf = sensors_get_features (detected_chip, &nr1);
        while (sf != NULL)
        {
            Ptr0<t_chipfeature> chip_feature = find_chipfeature (detected_chip, chip, sf);
            if (chip_feature)
                chip->chip_features.push_back(chip_feature.toPtr());
            sf = sensors_get_features (detected_chip, &nr1);
        }

        detected_chip = sensors_get_detected_chips (NULL, &num_sensorchips);
    }

    return 1;
}


/* -------------------------------------------------------------------------- */
void
refresh_lmsensors (const Ptr<t_chipfeature> &feature) {}


/* -------------------------------------------------------------------------- */
void
free_lmsensors_chip (t_chip *chip) {}
