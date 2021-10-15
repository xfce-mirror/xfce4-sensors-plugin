/* acpi.cc
 * Part of xfce4-sensors-plugin
 *
 * Copyright (c) 2004-2017 Fabian Nowak <timystery@arcor.de>
 * Copyright (c) 2021 Jan Ziak <0xe2.0x9a.0x9b@xfce.org>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Package includes */
#include <acpi.h>
#include <types.h>
#include <middlelayer.h>


/* -------------------------------------------------------------------------- */
static gchar*
strip_key_colon_spaces (gchar *string)
{
    gchar *position = string;

    g_return_val_if_fail (string != NULL, NULL);

    /* Skip everything before the ':' */
    if (strchr (string, ':'))
        position = strchr (string, ':') + 1;

    /* Skip all the spaces */
    while (*position == ' ')
        position++;

    return position;
}


/* -------------------------------------------------------------------------- */
#ifdef HAVE_SYSFS_ACPI
static void
cut_newline (gchar *string)
{
    g_return_if_fail (string != NULL);

    for (gint i = 0; string[i] != '\0'; i++)
    {
        if (string[i] == '\n')
        {
            string[i] = '\0';
            break;
        }
    }
}
#endif


/* -------------------------------------------------------------------------- */
gint
read_thermal_zone (t_chip *chip)
{
    gint res_value = -2;
#ifdef HAVE_SYSFS_ACPI
    gchar buffer[1024];
#else
    gchar *zone;
#endif

    g_return_val_if_fail (chip != NULL, res_value);

#ifdef HAVE_SYSFS_ACPI
    if ((chdir (SYS_PATH) == 0) && (chdir (SYS_DIR_THERMAL) == 0))
#else
    if ((chdir (ACPI_PATH) == 0) && (chdir (ACPI_DIR_THERMAL) == 0))
#endif
    {
        DIR *dir = opendir (".");
        if (!dir) {
            res_value = -1;
        }
        else
        {
            struct dirent *entry;
            gchar *filename;

            while ((entry = readdir (dir)) != NULL)
            {
                if (strncmp (entry->d_name, ".", 1) == 0)
                    continue;

    #ifdef HAVE_SYSFS_ACPI
                filename = g_strdup_printf ("/%s/%s/%s/%s", SYS_PATH, SYS_DIR_THERMAL, entry->d_name, SYS_FILE_THERMAL);
    #else
                filename = g_strdup_printf ("%s/%s/%s/%s", ACPI_PATH,
                                            ACPI_DIR_THERMAL, entry->d_name,
                                            ACPI_FILE_THERMAL);
    #endif
                FILE *file = fopen (filename, "r");
                if (file)
                {
                    DBG("parsing temperature file \"%s\"...\n", filename);
                    /* if (acpi_ignore_directory_entry (ptr_dirent))
                        continue; */

                    t_chipfeature *feature = g_new0 (t_chipfeature, 1);

                    feature->color_orNull = g_strdup("#0000B0");
                    feature->address = chip->chip_features->len;
                    feature->devicename = g_strdup (entry->d_name);
                    feature->name = g_strdup (feature->devicename);
                    feature->formatted_value = NULL; /*  Gonna refresh it in
                                                            sensors_get_wrapper or some
                                                            other functions */

    #ifdef HAVE_SYSFS_ACPI
                    if (fgets (buffer, 1024, file)!=NULL)
                    {
                        cut_newline (buffer);
                        feature->raw_value = strtod (buffer, NULL) / 1000.0;
                        DBG ("Raw-Value=%f\n", feature->raw_value);
                    }
    #else
                    zone = g_strdup_printf ("%s/%s", ACPI_DIR_THERMAL, entry->d_name);
                    feature->raw_value = get_acpi_zone_value (zone, ACPI_FILE_THERMAL);
                    g_free (zone);
    #endif

                    feature->valid = TRUE;
                    feature->min_value = 20.0;
                    feature->max_value = 60.0;
                    feature->cls = TEMPERATURE;

                    g_ptr_array_add (chip->chip_features, feature);

                    chip->num_features++; /* FIXME: actually I am just the same as
                        chip->chip_features->len */

                    fclose(file);
                }

                g_free (filename);
            }

            closedir (dir);

            res_value = 0;
        }
    }

    return res_value;
}


/* -------------------------------------------------------------------------- */
gdouble
get_fan_zone_value (const gchar *zone)
{
    gdouble res_value = 0;

    g_return_val_if_fail (zone != NULL, res_value);

    gchar *filename = g_strdup_printf ("%s/%s/%s/%s", ACPI_PATH, ACPI_DIR_FAN, zone, ACPI_FILE_FAN);
    DBG("filename=%s", filename);
    FILE *file = fopen (filename, "r");
    if (file) {
        gchar buffer[1024];
        while (fgets (buffer, 1024, file) != NULL)
        {
            if (strncmp (buffer, "status:", 7) == 0)
            {
                gchar *stripped_buffer = strip_key_colon_spaces(buffer);
                g_assert(stripped_buffer!=NULL);
                DBG ("tmp=%s", stripped_buffer);
                if (strncmp (stripped_buffer, "on", 2)==0)
                    res_value = 1.0;
                else
                    res_value = 0.0;

                break;
            }
        }
        fclose (file);
    }

    g_free (filename);

    return res_value;
}


/* -------------------------------------------------------------------------- */
gdouble
get_battery_zone_value (const gchar *zone)
{
    gdouble res_value = 0.0;
    gchar buffer[1024], *filename;

#ifndef HAVE_SYSFS_ACPI
    gchar *ptr_strippedbuffer;
#endif

    g_return_val_if_fail (zone != NULL, res_value);

#ifdef HAVE_SYSFS_ACPI
    filename = g_strdup_printf ("%s/%s/%s/%s", SYS_PATH, SYS_DIR_POWER, zone, SYS_FILE_ENERGY);
#else
    filename = g_strdup_printf ("%s/%s/%s/%s", ACPI_PATH, ACPI_DIR_BATTERY,
                                zone, ACPI_FILE_BATTERY_STATE);
#endif
    DBG("str_filename=%s\n", filename);
    FILE *file = fopen (filename, "r");
    if (file) {
#ifdef HAVE_SYSFS_ACPI
        if (fgets (buffer, 1024, file)!=NULL)
        {
            cut_newline (buffer);
            res_value = strtod (buffer, NULL) / 1000.0;
        }
#else
        while (fgets (buffer, 1024, file)!=NULL)
        {
            if (strncmp (buffer, "remaining capacity:", 19)==0)
            {
                ptr_strippedbuffer = strip_key_colon_spaces(buffer);
                g_assert(ptr_strippedbuffer!=NULL);
                res_value = strtod (ptr_strippedbuffer, NULL);
                break;
            }
        }
#endif
        fclose (file);
    }

    g_free (filename);

    return res_value;
}


/* -------------------------------------------------------------------------- */
gint
read_battery_zone (t_chip *chip)
{
    gint res_value = -1;

    g_return_val_if_fail(chip!=NULL, res_value);

#ifdef HAVE_SYSFS_ACPI
    if ((chdir (SYS_PATH) == 0) && (chdir (SYS_DIR_POWER) == 0)) {
#else
    if ((chdir (ACPI_PATH) == 0) && (chdir (ACPI_DIR_BATTERY) == 0)) {
#endif
        DIR *dir = opendir (".");
        struct dirent *entry;

        while (dir && (entry = readdir (dir)))
        {
            if (strncmp (entry->d_name, "BAT", 3) == 0)
            { /* have a battery subdirectory */
                gchar *filename;
                gchar buffer[1024];

#ifdef HAVE_SYSFS_ACPI
                filename = g_strdup_printf ("%s/%s/%s/%s", SYS_PATH, SYS_DIR_POWER, entry->d_name, SYS_POWER_MODEL_NAME);
#else
                filename = g_strdup_printf ("%s/%s/%s/%s", ACPI_PATH,
                                            ACPI_DIR_BATTERY, entry->d_name,
                                            ACPI_FILE_BATTERY_STATE);
#endif
                DBG ("str_filename=%s\n", filename);
                FILE *file = fopen (filename, "r");
                t_chipfeature *feature = g_new0 (t_chipfeature, 1);
                if (file) {
                    feature->address = chip->chip_features->len;
                    feature->devicename = g_strdup (entry->d_name);

#ifdef HAVE_SYSFS_ACPI
                    if (fgets (buffer, 1024, file)!=NULL)
                    {
                        cut_newline (buffer);
                        // Note for translators: As some laptops have several batteries such as the T440s,
                        // there might be some perturbation with the battery name here and BAT0/BAT1 for
                        // power/voltage. So we prepend BAT0/1 to the battery name as well, with the result
                        // being something like "BAT1 - 45N1127". Users can then rename the batteries to
                        // their own will while keeping consistency to their power/voltage features.
                        feature->name = g_strdup_printf (_("%s - %s"),entry->d_name, buffer);
                        DBG ("Name=%s\n", buffer);
                    }
#else
                    feature->name = g_strdup (feature->devicename);
#endif

                    feature->valid = TRUE;
                    feature->min_value = 0.0;
                    feature->raw_value = 0.0;
                    feature->cls = ENERGY;
                    feature->formatted_value = NULL;
                    feature->color_orNull = g_strdup("#0000B0");

#ifdef HAVE_SYSFS_ACPI
                    fclose (file);
                }
                g_free (filename);
                filename = g_strdup_printf ("%s/%s/%s/%s", SYS_PATH, SYS_DIR_POWER, entry->d_name, SYS_FILE_ENERGY);
                file = fopen (filename, "r");
                if (file) {

                    if (fgets (buffer, 1024, file)!=NULL)
                    {
                        cut_newline (buffer);
                        feature->raw_value = strtod (buffer, NULL);
                        DBG ("Raw-Value=%f\n", feature->raw_value);
                    }
                    fclose (file);
                }
                g_free (filename);
                filename = g_strdup_printf ("%s/%s/%s/%s", SYS_PATH, SYS_DIR_POWER, entry->d_name, SYS_FILE_ENERGY_MIN);
                file = fopen (filename, "r");
                if (file) {
                    if (fgets (buffer, 1024, file)!=NULL)
                    {
                        cut_newline (buffer);
                        feature->min_value = strtod (buffer, NULL) / 1000.0;
                        DBG ("Min-Value=%f\n", feature->min_value);
                    }
#else
                    while (fgets (buffer, 1024, file)!=NULL)
                    {
                        gchar *stripped_buffer;

                        if (strncmp (buffer, "design capacity low:", 20)==0)
                        {
                            stripped_buffer = strip_key_colon_spaces(buffer);
                            g_assert(stripped_buffer!=NULL);
                            feature->min_value = strtod (stripped_buffer, NULL);
                        }
                        else if (strncmp (buffer, "remaining capacity:", 19)==0)
                        {
                            stripped_buffer = strip_key_colon_spaces(buffer);
                            g_assert(stripped_buffer!=NULL);
                            feature->raw_value = strtod (stripped_buffer, NULL);
                        }
                    }
#endif

                    fclose (file);

                    g_ptr_array_add (chip->chip_features, feature);
                    chip->num_features++; /* FIXME: actually I am just the same
                                            as chip->chip_features->len */
                }
                else {
                    g_free (filename);
                    free_chipfeature (feature, NULL);
                    continue; /* what would we want to do with only
                                 a maxval and no real value inside? */
                }

                g_free (filename);

                get_battery_max_value (entry->d_name, feature);
            }

            res_value = 0;
        }

        if (dir)
            closedir (dir);
    }
    else
    {
        res_value = -2;
    }

    return res_value;
}


/* -------------------------------------------------------------------------- */
void
get_battery_max_value (const gchar *filename, t_chipfeature *feature)
{
    gchar *path_to_file;

    g_return_if_fail (filename != NULL);
    g_return_if_fail (feature != NULL);

#ifdef HAVE_SYSFS_ACPI
    path_to_file = g_strdup_printf ("%s/%s/%s/%s", SYS_PATH, SYS_DIR_POWER, filename, SYS_FILE_ENERGY_MAX);
#else
    path_to_file = g_strdup_printf ("%s/%s/%s/%s", ACPI_PATH,
                                    ACPI_DIR_BATTERY, filename,
                                    ACPI_FILE_BATTERY_INFO);
#endif
    DBG ("str_pathtofile=%s\n", path_to_file);
    FILE *file = fopen (path_to_file, "r");
    if (file)
    {
        gchar buffer[1024];

#ifdef HAVE_SYSFS_ACPI
        if (fgets (buffer, 1024, file)!=NULL)
        {
            cut_newline (buffer);
            feature->max_value = strtod (buffer, NULL) / 1000.0;
            DBG ("Max-Value=%f\n", feature->max_value);
        }
#else
        while (fgets (buffer, 1024, file) != NULL)
        {
            if (strncmp (buffer, "last full capacity:", 19) == 0)
            {
                gchar *stripped_buffer = strip_key_colon_spaces(buffer);
                g_assert(stripped_buffer!=NULL);
                feature->max_value = strtod (stripped_buffer, NULL);
                break;
            }
        }
#endif
        fclose (file);
    }

    g_free (path_to_file);
}


/* -------------------------------------------------------------------------- */
gint
read_fan_zone (t_chip *chip)
{
    gint res_value = -1;

    g_return_val_if_fail (chip != NULL, res_value);

    if ((chdir (ACPI_PATH) == 0) && (chdir (ACPI_DIR_FAN) == 0))
    {
        struct dirent *entry;

        DIR *dir = opendir (".");
        while (dir && (entry = readdir (dir)))
        {
            if (strncmp(entry->d_name, ".", 1)==0)
                continue;

            gchar *filename = g_strdup_printf ("%s/%s/%s/%s", ACPI_PATH,
                                               ACPI_DIR_FAN, entry->d_name,
                                               ACPI_FILE_FAN);
            FILE *file = fopen (filename, "r");
            if (file)
            {
                t_chipfeature *feature;

                /* if (acpi_ignore_directory_entry (de))
                    continue; */

                feature = g_new0 (t_chipfeature, 1);
                g_return_val_if_fail(feature != NULL, -1);

                feature->color_orNull = g_strdup("#0000B0");
                feature->address = chip->chip_features->len;
                feature->devicename = g_strdup (entry->d_name);
                feature->name = g_strdup (feature->devicename);
                feature->formatted_value = NULL; /* Gonna refresh it in
                                                    sensors_get_wrapper or some
                                                    other functions */
                feature->raw_value = get_fan_zone_value (entry->d_name);

                feature->valid = TRUE;
                feature->min_value = 0.0;
                feature->max_value = 2.0;
                feature->cls = STATE;

                g_ptr_array_add (chip->chip_features, feature);

                chip->num_features++; /* FIXME: actually I am just the same as
                    chip->chip_features->len */

                fclose(file);
            }

            g_free (filename);
            res_value = 0;
        }

        if (dir)
            closedir (dir);
    }
    else {
        res_value = -2;
    }
    return res_value;
}


/* -------------------------------------------------------------------------- */
gdouble
get_power_zone_value (const gchar *zone)
{
    gdouble res_value = 0;

    g_return_val_if_fail (zone != NULL, res_value);

    gchar *filename = g_strdup_printf ("%s/%s/%s/%s", SYS_PATH, SYS_DIR_POWER, zone, SYS_FILE_POWER);

    DBG("str_filename=%s\n", filename);
    FILE *file = fopen (filename, "r");
    if (file) {
        gchar buffer[1024];
        if (fgets (buffer, 1024, file)!=NULL)
        {
            cut_newline (buffer);
            res_value = strtod (buffer, NULL) / 1000000.0;
        }
        fclose (file);
    }

    g_free (filename);

    return res_value;
}


/* -------------------------------------------------------------------------- */
gdouble
get_voltage_zone_value (const gchar *zone)
{
    gdouble res_value = 0.0;

    g_return_val_if_fail (zone != NULL, res_value);

    gchar *filename = g_strdup_printf ("%s/%s/%s/%s", SYS_PATH, SYS_DIR_POWER, zone,  SYS_FILE_VOLTAGE);

    DBG("str_filename=%s\n", filename);
    FILE *file = fopen (filename, "r");
    if (file) {
        gchar buffer[1024];
        if (fgets (buffer, 1024, file)!=NULL)
        {
            cut_newline (buffer);
            res_value = strtod (buffer, NULL) / 1e6;
        }
        fclose (file);
    }

    g_free (filename);

    return res_value;
}


/* -------------------------------------------------------------------------- */
gint
read_power_zone (t_chip *chip)
{
    gint res_value = -1;

    g_return_val_if_fail (chip != NULL, res_value);

    if ((chdir (SYS_PATH) == 0) && (chdir (SYS_DIR_POWER) == 0)) {
        DIR *dir = opendir (".");
        struct dirent *entry;
        while (dir && (entry = readdir (dir)))
        {
            if (strncmp(entry->d_name, "BAT", 3)==0)
            { /* have a battery subdirectory */

                gchar *filename = g_strdup_printf ("%s/%s/%s/%s", SYS_PATH, SYS_DIR_POWER, entry->d_name, SYS_FILE_POWER);
                FILE *file = fopen (filename, "r");
                if (file) {
                    t_chipfeature *feature;

                    feature = g_new0 (t_chipfeature, 1);
                    g_return_val_if_fail(feature != NULL, -1);

                    feature->color_orNull = g_strdup("#00B0B0");
                    feature->address = chip->chip_features->len;
                    feature->devicename = g_strdup(entry->d_name);
                    // You might want to format this with a hyphen and without spacing, or with a dash; the result might be BAT1–Power or whatever fits your language most. Spaces allow line breaks over the tachometers.
                    feature->name = g_strdup_printf (_("%s - %s"),
                                                     // Power with unit Watts, not Energy with Joules or kWh
                                                     entry->d_name, _("Power"));
                    feature->formatted_value = NULL;
                    feature->raw_value = get_power_zone_value(entry->d_name);
                    feature->valid = TRUE;
                    feature->min_value = 0.0;
                    feature->max_value = 60.0; // a T440s charges with roughly 25 Watts
                    feature->cls = POWER;

                    g_ptr_array_add (chip->chip_features, feature);

                    chip->num_features++; /* FIXME: actually I am just the same as
                    chip->chip_features->len */

                    fclose (file);
                }
                g_free (filename);
            }

            res_value = 0;
        }

        if (dir)
            closedir (dir);
    }
    else
    {
        res_value = -2;
    }

    return res_value;
}


/* -------------------------------------------------------------------------- */
gint
read_voltage_zone (t_chip *chip)
{
    gint res_value = -1;

    g_return_val_if_fail (chip != NULL, res_value);

    if ((chdir (SYS_PATH) == 0) && (chdir (SYS_DIR_POWER) == 0)) {
        DIR *dir = opendir (".");
        struct dirent *entry;
        while (dir && (entry = readdir (dir)))
        {
            if (strncmp(entry->d_name, "BAT", 3)==0)
            { /* have a battery subdirectory */

                gchar *filename = g_strdup_printf ("%s/%s/%s/%s", SYS_PATH, SYS_DIR_POWER, entry->d_name, SYS_FILE_VOLTAGE);
                FILE *file = fopen (filename, "r");
                if (file) {
                    t_chipfeature *feature;
                    gchar *min_voltage, *zone;

                    feature = g_new0 (t_chipfeature, 1);
                    g_return_val_if_fail (feature != NULL, -1);

                    feature->color_orNull = g_strdup("#00B0B0");
                    feature->address = chip->chip_features->len;
                    feature->devicename = g_strdup(entry->d_name);
                    // You might want to format this with a hyphen and without spacing, or with a dash; the result might be BAT1–Voltage or whatever fits your language most. Spaces allow line breaks over the tachometers.
                    feature->name = g_strdup_printf (_("%s - %s"), entry->d_name, _("Voltage"));
                    feature->formatted_value = NULL;
                    feature->raw_value = get_voltage_zone_value(entry->d_name);
                    feature->valid = TRUE;
                    zone = g_strdup_printf ("%s/%s/%s/%s", SYS_PATH, SYS_DIR_POWER, entry->d_name, SYS_FILE_VOLTAGE_MIN);
                    min_voltage = get_acpi_value(zone);
                    g_free(zone);
                    feature->min_value = feature->raw_value;
                    if (min_voltage)
                    {
                        feature->min_value = strtod(min_voltage, NULL) / 1000000.0;
                        g_free(min_voltage);
                    }
                    feature->max_value = feature->raw_value; // a T440s charges with roughly 25 Watts
                    feature->cls = VOLTAGE;

                    g_ptr_array_add (chip->chip_features, feature);

                    chip->num_features++; /* FIXME: actually I am just the same as
                    chip->chip_features->len */

                    fclose (file);
                }
                g_free (filename);
            }

            res_value = 0;
        }

        if (dir)
            closedir (dir);
    }
    else
    {
        res_value = -2;
    }

    return res_value;
}


/* -------------------------------------------------------------------------- */
gint
initialize_ACPI (GPtrArray *chips)
{
    g_return_val_if_fail (chips != NULL, -1);

    t_chip *chip = g_new0 (t_chip, 1);
    g_return_val_if_fail (chip != NULL, -1);

    chip->name = g_strdup(_("ACPI")); /* to be displayed */

    gchar *acpi_info = get_acpi_info();
    chip->description = g_strdup_printf (_("ACPI v%s zones"), acpi_info);
    g_free(acpi_info);
    chip->sensorId = g_strdup ("ACPI"); /* used internally */

    chip->type = ACPI;

    sensors_chip_name *chip_name = g_new0 (sensors_chip_name, 1);
    g_return_val_if_fail (chip_name != NULL, -1);

    chip_name->prefix = g_strdup(_("ACPI"));

    chip->chip_name = (sensors_chip_name *) chip_name;

    chip->chip_features = g_ptr_array_new ();

    chip->num_features = 0;

    read_battery_zone (chip);
    read_thermal_zone (chip);
    read_fan_zone (chip);

#ifdef HAVE_SYSFS_ACPI
    read_power_zone (chip);
    read_voltage_zone (chip);
#endif

    g_ptr_array_add (chips, chip);

    return 4;
}


/* -------------------------------------------------------------------------- */
void
refresh_acpi (gpointer ptr_feature, gpointer unused)
{
    gchar *filename, *zone, *state;
#ifdef HAVE_SYSFS_ACPI
    FILE *file = NULL;
#endif

    auto feature = (t_chipfeature *) ptr_feature;
    g_return_if_fail(feature != NULL);

    switch (feature->cls) {
        case TEMPERATURE:
#ifdef HAVE_SYSFS_ACPI
            zone = g_strdup_printf ("%s/%s/%s/%s", SYS_PATH, SYS_DIR_THERMAL, feature->devicename, SYS_FILE_THERMAL);
            file = fopen(zone, "r");
            if (file)
            {
              gchar buffer[1024];
              if (fgets (buffer, sizeof(buffer), file)) /* automatically null-terminated */
              {
                cut_newline(buffer);
                feature->raw_value = strtod(buffer, NULL) / 1000.0;
              }
              fclose (file);
              file = NULL; /* avoid reuse after closing file */
            }
#else
            zone = g_strdup_printf ("%s/%s", ACPI_DIR_THERMAL, feature->devicename);
            feature->raw_value = get_acpi_zone_value (zone, ACPI_FILE_THERMAL);
#endif
            g_free (zone);
            break;

        case ENERGY:
            feature->raw_value = get_battery_zone_value (feature->devicename);
            break;

        case POWER:
            feature->raw_value = get_power_zone_value (feature->devicename);
            break;

        case VOLTAGE:
            feature->raw_value = get_voltage_zone_value (feature->devicename);
            break;

        case STATE:
            filename = g_strdup_printf ("%s/%s/%s/state", ACPI_PATH, ACPI_DIR_FAN, feature->devicename);

            state = get_acpi_value(filename); /* returned value is strdup'ped */
            if (!state)
            {
                DBG("Could not determine fan state.");
                feature->raw_value = 0.0;
            }
            else
            {
                feature->raw_value = strncmp(state, "on", 2)==0 ? 1.0 : 0.0;
                g_free (state);
            }
            g_free (filename);
            break;

        default:
            printf ("Unknown ACPI type. Please check your ACPI installation "
                    "and restart the plugin.\n");
    }
}


/* -------------------------------------------------------------------------- */
gint
acpi_ignore_directory_entry (struct dirent *entry)
{
    g_return_val_if_fail (entry != NULL, INT_MAX);
    return strcmp (entry->d_name, "temperature");
}


/* -------------------------------------------------------------------------- */
/**
 * Obtains ACPI version information.
 * Might forget some space or tab bytes due to g_strchomp.
 */
gchar *
get_acpi_info (void)
{
    gchar *filename = g_strdup_printf ("%s/%s", ACPI_PATH, ACPI_INFO);
    gchar *version = get_acpi_value (filename);
    g_free (filename);

    if (!version)
    {
        filename = g_strdup_printf ("%s/%s_", ACPI_PATH, ACPI_INFO);
        version = get_acpi_value (filename);
        g_free (filename);

        if (!version)
            version = get_acpi_value ("/sys/module/acpi/parameters/acpica_str_version");
    }

    /* sometimes, we obtain NULL str_version that can't be chomped then */
    if (version)
        version = g_strchomp (version);
    else
        version = g_strdup (_("<Unknown>"));

    return version;
}


/* -------------------------------------------------------------------------- */
/**
 * Note that zone will have to consist of two paths, e.g.
 * thermal_zone and THRM.
 */
gdouble
get_acpi_zone_value (const gchar *zone, const gchar *filename)
{
    gdouble res_value = 0.0;

    g_return_val_if_fail (zone != NULL, res_value);
    g_return_val_if_fail (filename != NULL, res_value);

    gchar *zone_filename = g_strdup_printf ("%s/%s/%s", ACPI_PATH, zone, filename);
    gchar *value = get_acpi_value (zone_filename);
    g_free(zone_filename);

    /* Return it as a double */
    if (value)
    {
        res_value = strtod (value, NULL);
        g_free (value);
    }

    return res_value;
}


/* -------------------------------------------------------------------------- */
gchar *
get_acpi_value (const gchar *filename)
{
    gchar *result = NULL;

    g_return_val_if_fail (filename != NULL, result);

    FILE *file = fopen (filename, "r");
    if (file)
    {
        gchar buffer[1024];
        if (fgets (buffer, sizeof(buffer), file)) /* appends null-byte character at end */
        {
            gchar *valueinstring = strip_key_colon_spaces (buffer);
            g_assert(valueinstring!=NULL); /* points to beginning of buffer at least */

            result = g_strdup (valueinstring);
        }

        fclose (file);
    }

    /* Have read the data */
    return result;
}


/* -------------------------------------------------------------------------- */
void
free_acpi_chip (gpointer ptr_chip)
{
    auto chip = (t_chip*) ptr_chip;

    g_return_if_fail (chip != NULL);
    g_return_if_fail (chip->chip_name != NULL);

    g_free (chip->chip_name->path);
    g_free (chip->chip_name->prefix);
}
