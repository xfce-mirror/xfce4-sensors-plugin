/*  Copyright 2004-2007 Fabian Nowak (timystery@arcor.de)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/* Note for programmers and editors: Try to use 4 spaces instead of Tab! */

#ifndef XFCE4_SENSORS_ACPI_H
#define XFCE4_SENSORS_ACPI_H

#define ACPI_PATH               "/proc/acpi"
#define ACPI_DIR_THERMAL        "thermal_zone"
#define ACPI_DIR_BATTERY        "battery"
#define ACPI_DIR_FAN            "fan"
#define ACPI_FILE_THERMAL       "temperature"
#define ACPI_FILE_BATTERY_STATE "state"
#define ACPI_FILE_BATTERY_INFO  "info"
#define ACPI_FILE_FAN           "state"

#define ACPI_INFO               "info_"

/* Gtk/Glib includes */
#include <glib.h>

/* Global includes */
#include <dirent.h>    /* directory listing and reading */

/* Package/local includes */
#include "types.h"

/*
 * Initialize ACPI by ?
 * @Return: Number of initialized chips
 * @Param: Pointer to array of chips
 */
int initialize_ACPI (GPtrArray *chips);


/**
 * Refreshs an ACPI chip's feature in sense of raw and formatted value
 * @Param chip_feature: Pointer to feature
 */
void refresh_acpi (gpointer chip_feature, gpointer data);


/**
 * Read a double value from zone. Calls get_acpi_value with prolonged path.
 * @Param zone: zone name under /proc/acpi including the subdir for one zone.
 * @Param file: file to read information from, e.g. info.
 * @Return: value read in first line of file after keyword and colon.
 */
double get_acpi_zone_value (char *zone, char *file);


/**
 * Read a double value from status file for fans.
 * When status is on, result is 1.0; else it is 0.0.
 * @Param zone: file to read information from, e.g. state.
 * @Return: valued read in any line starting with "status:", converted to 1 or 0
 */
double get_fan_zone_value (char *zone);

/**
 * Read a double value from special subzone denoted by name.
 * @Param name: name in the /pro/acpi/battery directory.
 * @Param chipfeature: pointer to chipfeature to get a max value.
 */
void get_battery_max_value (char *name, t_chipfeature *chipfeature);

/**
 * Read information from the thermal zone.
 * @Param chip: Pointer to already allocated chip, where values can be added.
 * @Return: 0 on success
 */
int read_thermal_zone (t_chip *chip);


/**
 * Read information from the battery zone.
 * @Param chip: Pointer to already allocated chip, where values can be added.
 * @Return: 0 on success
 */
int read_battery_zone (t_chip *chip);


/**
 * Read information from the fan zone.
 * @Param chip: Pointer to already allocated chip, where values can be added.
 * @Return: 0 on success
 */
int read_fan_zone (t_chip *chip);


/**
 * Returns the ACPI version number from /proc/acpi/info file.
 * @Return: versionnumber as string!
 */
char * get_acpi_info ();


/**
 * Get the string found in filename after the colon. To make a double out of
 * it, strtod is suitable.
 * @Param filename: Complete path to file to be inspected.
 * @Return: String of value found, "<Unknown>" otherwise.
 */
char * get_acpi_value (char *filename);


/**
 * Get the battery percentage from the battery information.
 * @Param zone: Complete zone path including both e.g. "battery" and "BAT0"
 * @Return double value of current battery power
 */
double get_battery_zone_value (char *zone);


/**
 * Indicates whether a given directory entry should be ignored as it's not
 *  "temperature".
 * @Param de: pointer to directory entry.
 * @Return: 1 on ignore, else 0
 */
int acpi_ignore_directory_entry (struct dirent *de);

#endif /* XFCE4_SENSORS_ACPI_H */
