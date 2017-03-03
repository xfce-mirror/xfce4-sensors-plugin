/* File: types.h
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

#ifndef XFCE4_SENSORS_TYPES_H
#define XFCE4_SENSORS_TYPES_H

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

/* Glib includes */
#include <glib.h>

/* Xfce includes */
#include <libxfce4util/libxfce4util.h>

#ifdef HAVE_LIBSENSORS
 #include <sensors/sensors.h>
#else
/**
 * sensors chip name structure from libsensors it is reused for the other
 * chiptypes as well
 */
 typedef struct sensors_chip_name {
  char *prefix;
  int bus; /* newer sensors.h has sensors_bus_id as struct{short,short} */
  int addr;
  char *path;    /* if dummy */
 } sensors_chip_name;
#endif


/**
 * temperature scale to show values in
 */
typedef enum {
    CELSIUS,
    FAHRENHEIT
} t_tempscale;


/**
 * type of chip, i.e, what is its purpose, feature, hardware?
 */
typedef enum {
    LMSENSOR,
    HDD,
    ACPI,
    GPU
} t_chiptype;


/**
 * Indicates whether chipfeature is a temperature, a voltage or a speed
 * value
 */
typedef enum {
    TEMPERATURE,
    VOLTAGE,
    SPEED,
    ENERGY,
    STATE,
    OTHER
} t_chipfeature_class;


/**
 * Information about a special feature on a chip
 */
typedef struct {
    gchar *name;
    gchar *devicename;
    double raw_value; /* unformatted sensor feature values */
    gchar *formatted_value; /* formatted (%f5.2) sensor feature values */
    float min_value;
    float max_value;
    gchar *color;
    gboolean show;
    gint address; /* specifies the mapping to the internal number in chip_name */
    gboolean valid;
    t_chipfeature_class class;
} t_chipfeature;


/**
 * Information about a whole chip, like asb-1-45
 */
typedef struct {
    gchar *sensorId;
    gchar *name;
    gchar *description;
    gint num_features;
    sensors_chip_name *chip_name;
    GPtrArray *chip_features;
    t_chiptype type;
} t_chip;

#endif /* XFCE4_SENSORS_TYPES_H */
