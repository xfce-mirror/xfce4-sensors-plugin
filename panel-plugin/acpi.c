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

#include "acpi.h"
#include "types.h"

#include <glib/gmessages.h>
#include <glib/gmem.h>
#include <glib/gprintf.h>
#include <glib/gstrfuncs.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

int initialize_ACPI (GPtrArray *chips)
{
    DIR *d;
    struct dirent *de;
    int entries = 0;
    t_chip *chip;
    double value;
    t_chipfeature *chipfeature;

    TRACE ("enters initialize_ACPI");

    chip = g_new0 (t_chip, 1);
    chip->name = g_strdup (_("ACPI"));
    chip->description = g_strdup (_("Advanced Configuration and Power Interface"));
    chip->sensorId = g_strdup("ACPI");

    chip->type = ACPI;

    chip->chip_name = (const sensors_chip_name *)
                            ( g_strdup(_("ACPI")), 0, 0, g_strdup(_("ACPI")) );

    chip->chip_features = g_ptr_array_new ();

    chip->num_features = 0;

    if ((chdir (ACPI_PATH) == 0) && (chdir ("thermal_zone") == 0)) {
        d = opendir (".");
        if (!d) return -1;

        while ((de = readdir (d))) {
            if (acpi_ignore_directory_entry (de))
                continue;
            entries++;

            chipfeature = g_new0 (t_chipfeature, 1);
            value = get_acpi_zone_value (de->d_name);
            chipfeature->raw_value = value;
            chipfeature->color = "#0000B0";
            chipfeature->formatted_value = g_strdup_printf ("%+5.1f", value);
            chipfeature->name = g_strdup (de->d_name);
            chipfeature->valid = TRUE;
            chipfeature->min_value = 20.0;
            chipfeature->max_value = 60.0;
            chipfeature->class = TEMPERATURE;

            g_ptr_array_add (chip->chip_features, chipfeature);

            chip->num_features++;
        }
    }

    g_ptr_array_add (chips, chip);

    TRACE ("leaves initialize_ACPI");

    return 4;
}

void
refresh_acpi (gpointer chip_feature, gpointer data)
{
    t_chipfeature *cf;

    TRACE ("enters refresh_acpi");

    g_assert(chip_feature!=NULL);

    cf = (t_chipfeature *) chip_feature;

    TRACE ("leaves refresh_acpi");
}


int
acpi_ignore_directory_entry (struct dirent *de)
{
    TRACE ("enters and leaves acpi_ignore_directory_entry");

    return !strcmp(de->d_name, ".") || !strcmp(de->d_name, "..");
}


char *
get_acpi_value (char *filename)
{
    FILE *file;
    char buf [1024];
    char *p;

    TRACE ("enters get_acpi_value for %s", filename);

    file = fopen (filename, "r");
    if (!file) return NULL;

    fgets (buf, 1024, file);
    fclose (file);

    p = buf;

    /* Skip everything before the ':' */
    while (*(p++)) {
        if (*p == ':') {
            break;
        }
    }
    p++;
    /* Skip all the spaces */
    while (*(p++)) {
        if (*p != ' ') {
            break;
        }
    }

    TRACE ("leaves get_acpi_value with %s", p);

    /* Have read the data */
    return g_strdup (p);
}


char *
get_acpi_info ()
{
    char *filename;

    TRACE ("enters get_acpi_info");

    filename = g_strdup_printf ("%s/%s", ACPI_PATH, ACPI_INFO);

    TRACE ("leaves get_acpi_info");

    return get_acpi_value (filename);
}


double
get_acpi_zone_value (char *zone)
{
    char *filename, *value;

    TRACE ("enters get_acpi_zone_value for %s", info);

    filename = g_strdup_printf ("%s/%s/%s/%s", ACPI_PATH, ACPI_DIR,
                                                        zone, ACPI_FILE);
    value = get_acpi_value (filename);

    if (!value) return 0.0;

    TRACE ("leaves get_acpi_zone_value");

    /* Return it as a double */
    return strtod (value, NULL);
}

