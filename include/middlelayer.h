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

#include <glib.h>

#include "sensors-interface-plugin.h"
#include "types.h"

/**
 * Initialize all sensors detected by iterating and calling init-routines
 * @param out_suppressmessage [out]: whether to suppress messages when reading the chip
 * @return Number of initialized features
 */
int initialize_all (std::vector<Ptr<t_chip>> &out_chips, bool *out_suppressmessage);


/**
 * Refresh all features of a chip
 */
void refresh_chip (const Ptr<t_chip> &chip, const Ptr<t_sensors> &data);


/**
 * Classifies sensor type
 * @param feature: Pointer to feature
 */
void categorize_sensor_type (const Ptr<t_chipfeature> &feature);


/**
 * Gets value of specified number in chip_name
 * @param chip: specifies bus and stuff of the sensors chip feature
 * @param idx_chipfeature: number of chipfeature to look for
 * @param out_suppressmessage: valid pointer to boolean indicating suppression of
 *                         messages, or NULL.
 * @return double value on success, or an empty Optional on error.
 */
Optional<double> sensor_get_value (const Ptr<t_chip> &chip, size_t idx_chipfeature, bool *out_suppressmessage);


/**
 * Clean up structures and call library routines for ending "session".
 */
void cleanup_interfaces ();


/**
 * Refreshes all chips at once.
 */
void refresh_all_chips (const std::vector<Ptr<t_chip>> &chips, const Ptr<t_sensors> &sensors);

#endif /* XFCE4_SENSORS_MIDDLELAYER_H */
