/* File: hddtemp.h
 *
 * Copyright 2004-2017 Fabian Nowak (timystery@arcor.de)
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

#ifndef XFCE4_SENSORS_HDDTEMP_H
#define XFCE4_SENSORS_HDDTEMP_H

/* Package includes */
#include <types.h>

/* Gtk/Glib includes */
#include <glib.h>

#define HDDTEMP_DISK_SLEEPING       ZERO_KELVIN /* must be larger than the remaining ones */
#define NO_VALID_HDDTEMP_PROGRAM    ZERO_KELVIN-1 /* the calls/communication to hddtemp don't work */
#define NO_VALID_TEMPERATURE_VALUE  ZERO_KELVIN-2 /* the value for a single disk is invalid */


/**
 * Initialize hddtemp by finding disks to monitor
 * @param arr_ptr_chips: Pointer to array of chips
 * @param ptr_suppressmessage: returns whether messages shall be suppressed
 * @return Number of initialized chips
 */
int initialize_hddtemp (GPtrArray *arr_ptr_chips, gboolean *ptr_suppressmessage);


/**
 * Refreshs a hddtemp chip's feature in sense of raw and formatted value
 *
 * @param ptr_chip_feature: Pointer to feature
 * @param ptr_sensors: Pointer to t_sensors or NULL
 */
void refresh_hddtemp (gpointer ptr_chip_feature, gpointer ptr_sensors);


/**
 * Gets the hddtemp value of a disk
 * @param str_disk: disk name
 * @param ptr_suppressmessage: returns whether messages shall be suppressed
 * @return Temperature of disk
 */
double get_hddtemp_value (char* str_disk, gboolean *ptr_suppressmessage);

#endif /* XFCE4_SENSORS_HDDTEMP_H */
