/* $Id$ */
/*
 *      middlelayer.h
 *
 *      Copyright 2006, 2007 Fabian Nowak <timytery@arcor.de>
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


#ifndef XFCE4_SENSORS_MIDDLELAYER_H
#define XFCE4_SENSORS_MIDDLELAYER_H

/* Package/Local includes */
#include "types.h"

/* Gtk/Glib includes */
#include <glib.h>

#define NO_VALID_HDDTEMP -2

/**
 * Initialize all sensors detected by iterating and calling init-routines
 * @Param chips: Double-pointer to array of chips
 * @Return Number of initialized features
 */
int initialize_all (GPtrArray **chips, gboolean *suppressmessage);


/**
 * Refresh all features of a chip
 *
 * @Param chip: Pointer to chip
 * @Param data: pointer to t_sensors or NULL;
 */
void refresh_chip (gpointer chip, gpointer data);


/**
 * Refresh all features of a chip
 * @Param chips: Pointer to chip pointers
 * @Param sensors: valid pointer to sensors structure or NULL
 */
/* seems to be pretty unused meanwhile ... */
/* void refresh_all_chips (GPtrArray *chips, t_sensors *sensors ); */


/**
 * Classifies sensor type
 * @Param chipfeature: Pointer to feature
 */
void categorize_sensor_type (t_chipfeature *chipfeature);


/**
 * Gets value of specified number in chip_name
 * @Param chip_name: takten from libsensors3, it specifies bus and stuff of
 * the sensors chip feature
 * @Param number: number of chipfeature to look for
 * @Param value: address where double value can be stored
 * @Param suppressmessage: valid pointer to boolean indicating suppression of
 *                         messages, or NULL.
 * @Return 0 on success, >0 else.
 */
int sensor_get_value (t_chip *chip, int number, double *value, gboolean *suppressmessage);


/**
 * Free data in chipfeatures
 */
void free_chipfeature (gpointer chipfeature, gpointer data);


/**
 * Free reamining structures in chips and associated chipfeatures
 */
void free_chip (gpointer chip, gpointer data);


/**
 * Clean up structures and call library routines for ending "session".
 */
void sensor_interface_cleanup ();

#endif /* XFCE4_SENSORS_MIDDLELAYER_H */
