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

#define ACPI_PATH    "/proc/acpi"
#define ACPI_DIR     "thermal_zone"
#define ACPI_FILE    "temperature"
#define ACPI_INFO    "info"

#include <glib/garray.h>

#include <dirent.h>    /* directory listing and reading */

/*
 * Initialize ACPI by ?
 * @Return: Number of initialized chips
 * @Param: Pointer to array of chips
 */
int initialize_ACPI (GPtrArray *chips);


/*
 * Refreshs an ACPI chip's feature in sense of raw and formatted value
 * @Param chip_feature: Pointer to feature
 */
void refresh_acpi (gpointer chip_feature, gpointer data);


double get_acpi_zone_value (char *zone);


char *get_acpi_info ();


char *get_acpi_value (char *filename);


int acpi_ignore_directory_entry (struct dirent *de);

#endif /* XFCE4_SENSORS_ACPI_H */
