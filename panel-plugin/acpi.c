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


static char *
strip_key_colon_spaces (char *buf)
{
    char *p;
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

    return p;
}


int
read_thermal_zone (t_chip *chip)
{
    DIR *d;
    FILE *file;
    char *zone, *filename;
    struct dirent *de;
    t_chipfeature *chipfeature;

    TRACE ("enters read_thermal_zone");

    if ((chdir (ACPI_PATH) == 0) && (chdir (ACPI_DIR_THERMAL) == 0))
    {
        d = opendir (".");
        if (!d) {
            closedir (d);
            return -1;
        }

        while ((de = readdir (d)))
        {
            //printf ("reading %s\n", de->d_name);
            if (strncmp(de->d_name, ".", 1)==0)
                continue;

            filename = g_strdup_printf ("%s/%s/%s/%s", ACPI_PATH,
                                        ACPI_DIR_THERMAL, de->d_name,
                                        ACPI_FILE_THERMAL);
            file = fopen (filename, "r");
            if (file)
            {
                //printf ("parsing temperature file...\n");
                /* if (acpi_ignore_directory_entry (de))
                    continue; */

                chipfeature = g_new0 (t_chipfeature, 1);

                chipfeature->color = g_strdup("#0000B0");
                chipfeature->address = chip->chip_features->len;
                chipfeature->devicename = g_strdup (de->d_name);
                chipfeature->name = g_strdup (chipfeature->devicename);
                chipfeature->formatted_value = NULL; /*  Gonna refresh it in
                                                        sensors_get_wrapper or some
                                                        other functions */
                zone = g_strdup_printf ("%s/%s", ACPI_DIR_THERMAL, de->d_name);
                chipfeature->raw_value = get_acpi_zone_value (zone, ACPI_FILE_THERMAL);
                g_free (zone);

                chipfeature->valid = TRUE;
                chipfeature->min_value = 20.0;
                chipfeature->max_value = 60.0;
                chipfeature->class = TEMPERATURE;

                g_ptr_array_add (chip->chip_features, chipfeature);

                chip->num_features++; /* FIXME: actually I am just the same as
                    chip->chip_features->len */

                fclose(file);
            }
            //else
                //printf ("not parsing temperature file...\n");

            g_free (filename);
        }

        closedir (d);
        TRACE ("leaves read_thermal_zone");

        return 0;
    }
    else {
        TRACE ("leaves read_thermal_zone");
        return -2;
    }
}


double get_fan_zone_value (char *zone)
{
    double value;

    FILE *file;
    char buf [1024], *filename, *tmp;

    TRACE ("enters get_fan_zone_value for %s", zone);

    value = 0.0;

    filename = g_strdup_printf ("%s/%s/%s/%s", ACPI_PATH, ACPI_DIR_FAN,
                                zone, ACPI_FILE_FAN);
    DBG("filename=%s", filename);
    file = fopen (filename, "r");
    if (file) {
        while (fgets (buf, 1024, file)!=NULL)
        {
            if (strncmp (buf, "status:", 7)==0)
            {
                tmp = strip_key_colon_spaces(buf);
                DBG ("tmp=%s", tmp);
                if (strncmp (tmp, "on", 2)==0)
                    value = 1.0;
                else
                    value = 0.0;

                break;
            }
        }
        /*  g_free (tmp); */ /* points to inside the buffer! */
        fclose (file);
    }

    g_free (filename);

    return value;
}

double get_battery_zone_value (char *zone)
{
    double value;

    FILE *file;
    char buf [1024], *filename, *tmp;

    TRACE ("enters get_battery_zone_value for %s", zone);

    value = 0.0;

    filename = g_strdup_printf ("%s/%s/%s/%s", ACPI_PATH, ACPI_DIR_BATTERY,
                                zone, ACPI_FILE_BATTERY_STATE);
    file = fopen (filename, "r");
    if (file) {
        while (fgets (buf, 1024, file)!=NULL)
        {
            if (strncmp (buf, "remaining capacity:", 19)==0)
            {
                tmp = strip_key_colon_spaces(buf);
                value = strtod (tmp, NULL);
                break;
            }
        }
        /*  g_free (tmp); */ /* points to inside the buffer! */
        fclose (file);
    }

    g_free (filename);

    return value;
}


int read_battery_zone (t_chip *chip)
{
    DIR *d;
    FILE *file;
    char *filename, *tmp, buf[1024];
    struct dirent *de;
    t_chipfeature *chipfeature;

    TRACE ("enters read_battery_zone");

    if ((chdir (ACPI_PATH) == 0) && (chdir (ACPI_DIR_BATTERY) == 0)) {
        d = opendir (".");
        if (!d) {
            closedir (d);
            return -1;
        }

        while ((de = readdir (d)))
        {
            if (strncmp(de->d_name, "BAT", 3)==0)
            { /* have a battery subdirectory */

                filename = g_strdup_printf ("%s/%s/%s/%s", ACPI_PATH,
                                            ACPI_DIR_BATTERY, de->d_name,
                                            ACPI_FILE_BATTERY_STATE);
                DBG ("filename=%s\n", filename);
                file = fopen (filename, "r");
                if (file) {
                    chipfeature = g_new0 (t_chipfeature, 1);
                    chipfeature->address = chip->chip_features->len;
                    chipfeature->devicename = g_strdup (de->d_name);
                    chipfeature->name = g_strdup (chipfeature->devicename);
                    chipfeature->valid = TRUE;
                    chipfeature->min_value = 0.0;
                    chipfeature->raw_value = 0.0;
                    chipfeature->class = ENERGY;
                    chipfeature->formatted_value = NULL;
                    chipfeature->color = g_strdup("#0000B0");
                    while (fgets (buf, 1024, file)!=NULL)
                    {
                        if (strncmp (buf, "design capacity low:", 20)==0)
                        {
                            tmp = strip_key_colon_spaces(buf);
                            chipfeature->min_value = strtod (tmp, NULL);
                        }
                        else if (strncmp (buf, "remaining capacity:", 19)==0)
                        {
                            tmp = strip_key_colon_spaces(buf);
                            chipfeature->raw_value = strtod (tmp, NULL);
                        }
                    }
                    /* g_free (tmp); */ /* points to inside of the buffer */
                    fclose (file);

                    /* chipfeature->raw_value = get_battery_zone_value (de->d_name); */
                    g_ptr_array_add (chip->chip_features, chipfeature);
                    chip->num_features++; /* FIXME: actually I am just the same
                                            as chip->chip_features->len */
                }
                else {
                    g_free (filename);
                    continue; /* because, what would we want to do with only
                                a maxval and no real value inside? */
                }

                g_free (filename);

                get_battery_max_value (de->d_name, chipfeature);

            }

        }

        closedir (d);
        TRACE ("leaves read_battery_zone");

        return 0;

    }
    else
    {
        TRACE ("leaves read_battery_zone");
        return -2;
    }
}


void
get_battery_max_value (char *name, t_chipfeature *chipfeature)
{
    FILE *file;
    char *filename, *tmp, buf[1024];

    TRACE ("enters get_battery_max_value");

    filename = g_strdup_printf ("%s/%s/%s/%s", ACPI_PATH,
                                            ACPI_DIR_BATTERY, name,
                                            ACPI_FILE_BATTERY_INFO);
    file = fopen (filename, "r");
    if (file)
    {
        while (fgets (buf, 1024, file)!=NULL)
        {
            if (strncmp (buf, "last full capacity:", 19)==0)
            {

                tmp = strip_key_colon_spaces(buf);
                chipfeature->max_value = strtod (tmp, NULL);
            }
        }
        fclose (file);
    }

    g_free (filename);

    TRACE ("leaves get_battery_max_value");
}


int read_fan_zone (t_chip *chip)
{
    DIR *d;
    FILE *file;
    char *filename;
    struct dirent *de;
    t_chipfeature *chipfeature;

    TRACE ("enters read_fan_zone");

    if ((chdir (ACPI_PATH) == 0) && (chdir (ACPI_DIR_FAN) == 0))
    {
        d = opendir (".");
        if (!d) {
            closedir (d);
            return -1;
        }

        while ((de = readdir (d)))
        {
            //printf ("reading %s\n", de->d_name);
            if (strncmp(de->d_name, ".", 1)==0)
                continue;

            filename = g_strdup_printf ("%s/%s/%s/%s", ACPI_PATH,
                                        ACPI_DIR_FAN, de->d_name,
                                        ACPI_FILE_FAN);
            file = fopen (filename, "r");
            if (file)
            {
                //printf ("parsing temperature file...\n");
                /* if (acpi_ignore_directory_entry (de))
                    continue; */

                chipfeature = g_new0 (t_chipfeature, 1);

                chipfeature->color = g_strdup("#0000B0");
                chipfeature->address = chip->chip_features->len;
                chipfeature->devicename = g_strdup (de->d_name);
                chipfeature->name = g_strdup (chipfeature->devicename);
                chipfeature->formatted_value = NULL; /*  Gonna refresh it in
                                                        sensors_get_wrapper or some
                                                        other functions */
                chipfeature->raw_value = get_fan_zone_value (de->d_name);

                chipfeature->valid = TRUE;
                chipfeature->min_value = 0.0;
                chipfeature->max_value = 2.0;
                chipfeature->class = STATE;

                g_ptr_array_add (chip->chip_features, chipfeature);

                chip->num_features++; /* FIXME: actually I am just the same as
                    chip->chip_features->len */

                fclose(file);
            }
            //else
                //printf ("not parsing temperature file...\n");

            g_free (filename);
        }

        closedir (d);
        TRACE ("leaves read_fan_zone");

        return 0;
    }
    else {
        TRACE ("leaves read_fan_zone");
        return -2;
    }
    return -7;
}


int initialize_ACPI (GPtrArray *chips)
{
    t_chip *chip;

    TRACE ("enters initialize_ACPI");

    chip = g_new0 (t_chip, 1);
    chip->name = _("ACPI"); /* to be displayed */
    /* chip->description = _("Advanced Configuration and Power Interface"); */
    chip->description = g_strdup_printf (_("ACPI v%s zones"), get_acpi_info());
    chip->sensorId = "ACPI"; /* used internally */

    chip->type = ACPI;

    chip->chip_name = (const sensors_chip_name *)
                            ( g_strdup(_("ACPI")), 0, 0, g_strdup(_("ACPI")) );

    chip->chip_features = g_ptr_array_new ();

    chip->num_features = 0;

    read_battery_zone (chip);
    read_thermal_zone (chip);
    read_fan_zone (chip);

    g_ptr_array_add (chips, chip);

    /* int i = 0;
    t_chipfeature *chipfeature;
    printf ("chip->chip_features->len=%d\n", chip->chip_features->len);
    while (i<chip->chip_features->len ) {
        chipfeature = g_ptr_array_index(chip->chip_features, i++);
        g_printf ("chips val=%f\n", chipfeature->raw_value);
    } */

    TRACE ("leaves initialize_ACPI");

    return 4;
}

void
refresh_acpi (gpointer chip_feature, gpointer data)
{
    char *file, *zone, *state;
    t_chipfeature *cf;

    TRACE ("enters refresh_acpi");

    g_assert(chip_feature!=NULL);

    cf = (t_chipfeature *) chip_feature;

    switch (cf->class) {
        case TEMPERATURE:
            zone = g_strdup_printf ("%s/%s", ACPI_DIR_THERMAL, cf->devicename);
            cf->raw_value = get_acpi_zone_value (zone, ACPI_FILE_THERMAL);
            g_free (zone);
            /* g_free (cf->formatted_value);
            cf->formatted_value = g_strdup_printf (_("%+5.1f °C"), cf->raw_value); */
            break;

        case ENERGY:
            /* zone = g_strdup_printf ("%s/%s", ACPI_DIR_BATTERY, cf->devicename); */
            cf->raw_value = get_battery_zone_value (cf->devicename); /* zone */
            /* g_free (zone); */
            /*  g_free (cf->formatted_value);
            cf->formatted_value = g_strdup_printf (_("%.0f mWh"), cf->raw_value); */
            break;

        case STATE:
            file = g_strdup_printf ("%s/%s/%s/state", ACPI_PATH, ACPI_DIR_FAN, cf->devicename);
            
            state = get_acpi_value(file);
            if (state==NULL)
            {
            	DBG("Could not determine fan state.");
            	cf->raw_value = 0.0;
			}
			else
			{
            	cf->raw_value = strncmp(state, "on", 2)==0 ? 1.0 : 0.0;
            	/* g_free (state); Anyone with a fan state please check that, should be necessary to free this string as well */
			}
            g_free (file);
            
            /* g_free (cf->formatted_value);
            cf->formatted_value = g_strdup_printf (_("%.0f"), cf->raw_value); */
            break;

        default:
            printf ("Unknown ACPI type. Please check your ACPI installation "
                    "and restart the plugin.\n");
    }

    TRACE ("leaves refresh_acpi");
}


int
acpi_ignore_directory_entry (struct dirent *de)
{
    TRACE ("enters and leaves acpi_ignore_directory_entry");

    /*return !strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."); */
    return strcmp (de->d_name, "temperature");
}


char *
get_acpi_info ()
{
    char *filename, *version;

    TRACE ("enters get_acpi_info");

    filename = g_strdup_printf ("%s/%s", ACPI_PATH, ACPI_INFO);

    TRACE ("leaves get_acpi_info");

    version = get_acpi_value (filename);
    if (version!=NULL)
        version = g_strchomp (version);

    if (version==NULL)
        version = _("<Unknown>");

    return version;
}


/* Note that zone will have to consist of two paths, e.g.
 *  thermal_zone and THRM.
 */
double
get_acpi_zone_value (char *zone, char *file)
{
    char *filename, *value;
    double retval;

    TRACE ("enters get_acpi_zone_value for %s/%s", zone, file);

    filename = g_strdup_printf ("%s/%s/%s", ACPI_PATH, zone, file);
    value = get_acpi_value (filename);

    TRACE ("leaves get_acpi_zone_value with correctly converted value");

    /* Return it as a double */
    retval = strtod (value, NULL);
    g_free (value);

    return retval;
}


/**
 * Get the value from inside an acpi's file.
 * @param filename An absolute filename, most likely starting with /proc/acpi...
 * @return value found inside as a character
 */
char *
get_acpi_value (char *filename)
{
    FILE *file;
    char buf [1024], *p;

    TRACE ("enters get_acpi_value for %s", filename);

    file = fopen (filename, "r");
    if (!file)
        return NULL;

    (void) fgets (buf, 1024, file);
    fclose (file);

    p = strip_key_colon_spaces (buf);

    TRACE ("leaves get_acpi_value with %s", p);

    /* Have read the data */
    return g_strdup (p);
}

