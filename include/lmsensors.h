/* File: lmsensors.h
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

#ifndef XFCE4_SENSORS_LMSENSORS_H
#define XFCE4_SENSORS_LMSENSORS_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* Gtk/Glib includes */
#include <glib.h>

/* Global includes */
#include <sensors/sensors.h>

/* Package/local includes */
#include "types.h"

/**
 * Initialize libsensors by reading sensor config and other stuff
 * @param chips: Pointer to array of chips
 * @return Number of found chip_features
 */
int initialize_libsensors (GPtrArray *chips);


/**
 * Refreshs an lmsensors chip's feature in sense of raw and formatted value
 * @param chip_feature: Pointer to feature
 * @param unused: pointer to sensors structure
 */
void refresh_lmsensors (gpointer chip_feature, gpointer unused);


/**
 * Get the value of subsensor/feature that is number in array of sensors.
 * @param chip_name: Structure of sensor description.
 * @param idx_feature: number of feature to read the value from
 * @param out_value: pointer where the feature value is to be stored
 * @return 0 on success
 */
int sensors_get_feature_wrapper (const sensors_chip_name *chip_name,
                                 int idx_feature, double *out_value);


/**
 * Free the additionally allocated structures in the sensors_chip_name
 * according to the version of libsensors.
 * @param chip: Pointer to t_chip
 */
void free_lmsensors_chip (gpointer chip);

#endif /* XFCE4_SENSORS_LMSENSORS_H */
