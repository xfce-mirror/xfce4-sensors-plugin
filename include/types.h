/* File: types.h
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

#ifndef XFCE4_SENSORS_TYPES_H
#define XFCE4_SENSORS_TYPES_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib.h>
#include <libxfce4util/libxfce4util.h>

#ifdef HAVE_LIBSENSORS
#include <sensors/sensors.h>
#else
/**
 * sensors chip name structure from libsensors it is reused for the other
 * chiptypes as well
 */
struct sensors_chip_name {
    /** first part of textual sensor's chip name */
    char *prefix;

    /**
     * lm sensors have several "busses"; this selects a special bus, kind of
     *  array access.
     *  newer sensors.h has sensors_bus_id as struct{short,short}
     */
    int bus;

    /** address of the selected chip at the bus */
    int addr;

    /** path? unused in sensors plugin! */
    char *path;    /* if dummy */
};
#endif

G_BEGIN_DECLS

#define ZERO_KELVIN -273 /*.15 */

/**
 * temperature scale to show values in
 */
enum t_tempscale {
    CELSIUS,
    FAHRENHEIT
};


/**
 * type of chip, i.e, what is its purpose, feature, hardware?
 */
enum t_chiptype {
    LMSENSOR,
    HDD,
    ACPI,
    GPU
};


/**
 * Indicates whether chipfeature is a temperature, a voltage or a speed
 * value
 */
enum t_chipfeature_class {
    TEMPERATURE,
    VOLTAGE,
    SPEED,
    ENERGY,
    STATE,
    POWER,
    CURRENT,
    OTHER
};


/**
 * Information about a special feature on a chip
 */
struct t_chipfeature {
    /** name of chipfeature */
    gchar *name;

    /** underlying device */
    gchar *devicename;

    /** unformatted sensor feature values */
    double raw_value;

    /** formatted (%f5.2) sensor feature values */
    gchar *formatted_value;

    /** minimum value, used for visualization */
    float min_value;

    /** maximum value, used for visualization */
    float max_value;

    /** color for visualization */
    gchar *color_orNull;

    /** whether to show the value (and name) */
    gboolean show;

    /** specifies the mapping to the internal number in chip_name */
    gint address;

    /** is the chipfeature valid at all? */
    gboolean valid;

    /** class of chipfeature */
    t_chipfeature_class cls;
};


/**
 * Information about a whole chip, like asb-1-45
 */
struct t_chip {
    /** ID of the sensors chip */
    gchar *sensorId;

    /** name of the sensors chip */
    gchar *name;

    /** description of the sensors chip */
    gchar *description;

    /** number of known features for this chip */
    gint num_features;

    /** pointer to libsensors chip_name structure */
    sensors_chip_name *chip_name;

    /** array of pointers to child chip_features */
    GPtrArray *chip_features;

    /** chiptype, required for middlelayer to distinguish */
    t_chiptype type;
};

G_END_DECLS

#endif /* XFCE4_SENSORS_TYPES_H */
