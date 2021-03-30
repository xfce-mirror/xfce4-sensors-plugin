/* File: middlelayer.h
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

#ifndef XFCE4_SENSORS_MIDDLELAYER_H
#define XFCE4_SENSORS_MIDDLELAYER_H

/* Package/Local includes in same folder */
#include "sensors-interface-plugin.h" // includes ....common.h
#include "types.h"

/* Gtk/Glib includes */
#include <glib.h>

/**
 * Initialize all sensors detected by iterating and calling init-routines
 * @param out_chips: Double-pointer to pointer array of chips
 * @param out_suppressmessage [out]: whether to suppress messages when reading the chip
 * @return Number of initialized features
 */
int initialize_all (GPtrArray **out_chips, gboolean *out_suppressmessage);


/**
 * Refresh all features of a chip
 * @param chip: Pointer to chip
 * @param data: pointer to t_sensors or NULL;
 */
void refresh_chip (gpointer chip, gpointer data);


/**
 * Classifies sensor type
 * @param feature: Pointer to feature
 */
void categorize_sensor_type (t_chipfeature *feature);


/**
 * Gets value of specified number in chip_name
 * @param chip: specifies bus and stuff of the sensors chip feature
 * @param idx_chipfeature: number of chipfeature to look for
 * @param out_value: address where double value can be stored
 * @param out_suppressmessage: valid pointer to boolean indicating suppression of
 *                         messages, or NULL.
 * @return 0 on success, >0 else.
 */
int sensor_get_value (t_chip *chip, int idx_chipfeature, double *out_value,
                      gboolean *out_suppressmessage);


/**
 * Free data in chipfeatures
 * @param chip_feature: pointer to chipfeature to free
 * @param unused: currently unused
 */
void free_chipfeature (gpointer chip_feature, gpointer unused);


/**
 * Free remaining structures in chips and associated chipfeatures
 * @param chip: pointer to chip to free
 * @param unused: currently unused
 */
void free_chip (gpointer chip, gpointer unused);


/**
 * Clean up structures and call library routines for ending "session".
 */
void cleanup_interfaces (void);


/**
 * Refreshes all chips at once.
 * @param chips: Pointer to pointer array to chips
 * @param sensors: pointer to sensors structure
 */
void refresh_all_chips (GPtrArray *chips, t_sensors *sensors);

#endif /* XFCE4_SENSORS_MIDDLELAYER_H */
