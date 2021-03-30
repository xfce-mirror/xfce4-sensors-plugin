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
 * @param arr_ptr_chips: Pointer to array of chips
 * @return Number of found chip_features
 */
int initialize_libsensors (GPtrArray *arr_ptr_chips);


/**
 * Refreshs an lmsensors chip's feature in sense of raw and formatted value
 * @param ptr_chip_feature: Pointer to feature
 * @param ptr_unused: pointer to sensors structure
 */
void refresh_lmsensors (gpointer ptr_chip_feature, gpointer ptr_unused);


/**
 * Get the value of subsensor/feature that is number in array of sensors.
 * @param ptr_sensorschipname: Structure of sensor description.
 * @param idx_feature: number of feature to read the value from
 * @param ptr_returnvalue: pointer where the double feature value is to be stored
 * @return 0 on success
 */
int sensors_get_feature_wrapper (const sensors_chip_name *ptr_sensorschipname,
                                 int idx_feature, double *ptr_returnvalue);


/**
 * Free the additionally allocated structures in the sensors_chip_name
 * according to the version of libsensors.
 * @param ptr_chip: Pointer to t_chip
 */
void free_lmsensors_chip (gpointer ptr_chip);

#endif /* XFCE4_SENSORS_LMSENSORS_H */
