/* File: acpi.h
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

#ifndef XFCE4_SENSORS_ACPI_H
#define XFCE4_SENSORS_ACPI_H

#include <dirent.h>
#include <glib.h>

#include "types.h"

#define ACPI_PATH               "/proc/acpi"
#define ACPI_DIR_THERMAL        "thermal_zone"
#define ACPI_DIR_BATTERY        "battery"
#define ACPI_DIR_FAN            "fan"
#define ACPI_FILE_THERMAL       "temperature"
#define ACPI_FILE_BATTERY_STATE "state"
#define ACPI_FILE_BATTERY_INFO  "info"
#define ACPI_FILE_FAN           "state"
#define ACPI_INFO               "info"

#define SYS_PATH "/sys/class/"
#define SYS_POWER_MODEL_NAME "model_name"
#define SYS_DIR_THERMAL "thermal"
#define SYS_FILE_THERMAL "temp"
#define SYS_DIR_POWER "power_supply"
#define SYS_FILE_ENERGY "energy_now"
#define SYS_FILE_ENERGY_MIN "alarm"
#define SYS_FILE_ENERGY_MAX "energy_full"
#define SYS_FILE_POWER "power_now"
#define SYS_FILE_VOLTAGE "voltage_now"
#define SYS_FILE_VOLTAGE_MIN "voltage_min_design"

/**
 * Publish ACPI chip by appending chip data to the argument 'chips'.
 * @param chips: Pointer to an array of chips
 * @return Number of initialized chips
 */
gint initialize_ACPI (GPtrArray *chips);


/**
 * Refreshs an ACPI chip's feature in sense of raw and formatted value
 * @param chip_feature: Pointer to feature
 * @param unused: reserved for future use
 */
void refresh_acpi (gpointer chip_feature, gpointer unused);


/**
 * Read a double value from zone. Calls get_acpi_value with prolonged path.
 * @param zone: zone name under /proc/acpi including the subdir for one zone.
 * @param filename: file to read information from, e.g. info.
 * @return value from first line of file after keyword and colon.
 */
gdouble get_acpi_zone_value (const gchar *zone, const char *filename);


/**
 * Read a double value from status file for fans.
 * When status is on, result is 1.0; else it is 0.0.
 * @param zone_name: file to read information from, e.g. state.
 * @return valued from any line starting with "status:", converted to 1 or 0
 */
double get_fan_zone_value (const gchar *zone_name);


/**
 * Read a double value from special battery subzone denoted by the filename.
 * @param filename: name in the /pro/acpi/battery directory.
 * @param feature: pointer to chipfeature to get a max value.
 */
void get_battery_max_value (const gchar *filename, t_chipfeature *feature);


/**
 * Read a double value from status file for battery power.
 * When status is on, result is 1.0; else it is 0.0.
 * @param zone_name: file to read information from, e.g. state.
 * @return valued from any line starting with "status:", converted to 1 or 0
 */
double get_power_zone_value (const gchar *zone_name);


/**
 * Read a double value from status file for battery power.
 * When status is on, result is 1.0; else it is 0.0.
 * @param zone_name: file to read information from, e.g. state.
 * @return valued from any line starting with "status:", converted to 1 or 0
 */
double get_voltage_zone_value (const gchar *zone_name);


/**
 * Read information from the thermal zone.
 * @param chip: Pointer to already allocated chip, where values can be added.
 * @return 0 on success
 */
gint read_thermal_zone (t_chip *chip);


/**
 * Read information from the battery zone.
 * @param chip: Pointer to already allocated chip, where values can be added.
 * @return 0 on success
 */
gint read_battery_zone (t_chip *chip);


/**
 * Read information from the fan zone.
 * @param chip: Pointer to already allocated chip, where values can be added.
 * @return 0 on success
 */
gint read_fan_zone (t_chip *chip);


/**
 * Read information from the fan zone.
 * @param chip: Pointer to already allocated chip, where values can be added.
 * @return 0 on success
 */
gint read_power_zone (t_chip *chip);


/**
 * Read information from the fan zone.
 * @param chip: Pointer to already allocated chip, where values can be added.
 * @return 0 on success
 */
gint read_voltage_zone (t_chip *chip);


/**
 * Returns the ACPI version number from /proc/acpi/info file.
 * @return versionnumber as string!
 */
char* get_acpi_info (void);


/**
 * Get the string found in filename after the colon. To make a double out of
 * it, strtod is suitable.
 * @param filename: Complete path to file to be inspected.
 * @return String of value found, "<Unknown>" otherwise.
 */
char* get_acpi_value (const gchar *filename);


/**
 * Get the battery percentage from the battery information.
 * @param zone: Complete zone path including both e.g. "battery" and "BAT0"
 * @return double value of current battery power
 */
gdouble get_battery_zone_value (const gchar *zone);


/**
 * Indicates whether a given directory entry should be ignored as it's not
 *  "temperature".
 * @param entry: pointer to directory entry.
 * @return 1 on ignore, else 0
 */
gint acpi_ignore_directory_entry (struct dirent *entry);


/**
 * Free the additionally allocated structures in the sensors_chip_name
 * according to the version of libsensors.
 * @param chip: Pointer to t_chip
 */
void free_acpi_chip (gpointer chip);

#endif /* XFCE4_SENSORS_ACPI_H */
