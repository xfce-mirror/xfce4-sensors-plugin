/* acpi.cc
 * Part of xfce4-sensors-plugin
 *
 * Copyright (c) 2004-2017 Fabian Nowak <timystery@arcor.de>
 * Copyright (c) 2021-2022 Jan Ziak <0xe2.0x9a.0x9b@xfce.org>
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
#include "xfce4++/util.h"

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


/* -------------------------------------------------------------------------- */
gint
read_thermal_zone (const Ptr<t_chip> &chip)
{
    gint res_value = -2;

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

            while ((entry = readdir (dir)) != NULL)
            {
                if (strncmp (entry->d_name, ".", 1) == 0)
                    continue;

    #ifdef HAVE_SYSFS_ACPI
                auto filename = xfce4::sprintf ("/%s/%s/%s/%s", SYS_PATH, SYS_DIR_THERMAL, entry->d_name, SYS_FILE_THERMAL);
    #else
                auto filename = xfce4::sprintf ("%s/%s/%s/%s", ACPI_PATH, ACPI_DIR_THERMAL, entry->d_name, ACPI_FILE_THERMAL);
    #endif
                FILE *file = fopen (filename.c_str(), "r");
                if (file)
                {
                    DBG("parsing temperature file \"%s\"...\n", filename);
                    /* if (acpi_ignore_directory_entry (ptr_dirent))
                        continue; */

                    auto feature = xfce4::make<t_chipfeature>();

                    feature->color_orEmpty = "#0000B0";
                    feature->address = chip->chip_features.size();
                    feature->devicename = entry->d_name;
                    feature->name = feature->devicename;
                    feature->formatted_value = ""; /*  Gonna refresh it in sensors_get_wrapper or some other functions */

    #ifdef HAVE_SYSFS_ACPI
                    gchar buffer[1024];
                    if (fgets (buffer, 1024, file)!=NULL)
                    {
                        cut_newline (buffer);
                        feature->raw_value = strtod (buffer, NULL) / 1000.0;
                        DBG ("Raw-Value=%f\n", feature->raw_value);
                    }
    #else
                    auto zone = xfce4::sprintf ("%s/%s", ACPI_DIR_THERMAL, entry->d_name);
                    feature->raw_value = get_acpi_zone_value (zone, ACPI_FILE_THERMAL);
    #endif

                    feature->valid = true;
                    feature->min_value = 20.0;
                    feature->max_value = 60.0;
                    feature->cls = TEMPERATURE;

                    chip->chip_features.push_back(feature);

                    fclose(file);
                }
            }

            closedir (dir);

            res_value = 0;
        }
    }

    return res_value;
}


/* -------------------------------------------------------------------------- */
gdouble
get_fan_zone_value (const std::string &zone)
{
    gdouble res_value = 0;

    std::string filename = xfce4::sprintf ("%s/%s/%s/%s", ACPI_PATH, ACPI_DIR_FAN, zone.c_str(), ACPI_FILE_FAN);
    FILE *file = fopen (filename.c_str(), "r");
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

    return res_value;
}


/* -------------------------------------------------------------------------- */
gdouble
get_battery_zone_value (const std::string &zone)
{
    gdouble res_value = 0.0;
    std::string filename;

#ifdef HAVE_SYSFS_ACPI
    filename = xfce4::sprintf ("%s/%s/%s/%s", SYS_PATH, SYS_DIR_POWER, zone.c_str(), SYS_FILE_ENERGY);
#else
    filename = xfce4::sprintf ("%s/%s/%s/%s", ACPI_PATH, ACPI_DIR_BATTERY, zone.c_str(), ACPI_FILE_BATTERY_STATE);
#endif
    FILE *file = fopen (filename.c_str(), "r");
    if (file) {
        gchar buffer[1024];

#ifdef HAVE_SYSFS_ACPI
        if (fgets (buffer, 1024, file) != NULL)
        {
            cut_newline (buffer);
            res_value = strtod (buffer, NULL) / 1000.0;
        }
#else
        while (fgets (buffer, 1024, file) != NULL)
        {
            if (strncmp (buffer, "remaining capacity:", 19) == 0)
            {
                gchar *stripped_buffer = strip_key_colon_spaces(buffer);
                g_assert(stripped_buffer != NULL);
                res_value = strtod (stripped_buffer, NULL);
                break;
            }
        }
#endif
        fclose (file);
    }

    return res_value;
}


/* -------------------------------------------------------------------------- */
gint
read_battery_zone (const Ptr<t_chip> &chip)
{
    gint res_value = -1;

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
                std::string filename;
                gchar buffer[1024];

                auto feature = xfce4::make<t_chipfeature>();

#ifdef HAVE_SYSFS_ACPI
                filename = xfce4::sprintf ("%s/%s/%s/%s", SYS_PATH, SYS_DIR_POWER, entry->d_name, SYS_POWER_MODEL_NAME);
#else
                filename = xfce4::sprintf ("%s/%s/%s/%s", ACPI_PATH, ACPI_DIR_BATTERY, entry->d_name, ACPI_FILE_BATTERY_STATE);
#endif
                FILE *file = fopen (filename.c_str(), "r");
                if (file) {
                    feature->address = chip->chip_features.size();
                    feature->devicename = entry->d_name;

#ifdef HAVE_SYSFS_ACPI
                    if (fgets (buffer, 1024, file)!=NULL)
                    {
                        cut_newline (buffer);
                        // Note for translators: As some laptops have several batteries such as the T440s,
                        // there might be some perturbation with the battery name here and BAT0/BAT1 for
                        // power/voltage. So we prepend BAT0/1 to the battery name as well, with the result
                        // being something like "BAT1 - 45N1127". Users can then rename the batteries to
                        // their own will while keeping consistency to their power/voltage features.
                        feature->name = xfce4::sprintf (_("%s - %s"),entry->d_name, buffer);
                        DBG ("Name=%s\n", buffer);
                    }
#else
                    feature->name = feature->devicename;
#endif

                    feature->valid = true;
                    feature->min_value = 0.0;
                    feature->raw_value = 0.0;
                    feature->cls = ENERGY;
                    feature->formatted_value = "";
                    feature->color_orEmpty = "#0000B0";

#ifdef HAVE_SYSFS_ACPI
                    fclose (file);
                }

                filename = xfce4::sprintf ("%s/%s/%s/%s", SYS_PATH, SYS_DIR_POWER, entry->d_name, SYS_FILE_ENERGY);
                file = fopen (filename.c_str(), "r");
                if (file) {
                    if (fgets (buffer, 1024, file)!=NULL)
                    {
                        cut_newline (buffer);
                        feature->raw_value = strtod (buffer, NULL);
                        DBG ("Raw-Value=%f\n", feature->raw_value);
                    }
                    fclose (file);
                }

                filename = xfce4::sprintf ("%s/%s/%s/%s", SYS_PATH, SYS_DIR_POWER, entry->d_name, SYS_FILE_ENERGY_MIN);
                file = fopen (filename.c_str(), "r");
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

                    chip->chip_features.push_back(feature);
                }
                else {
                    continue; /* what would we want to do with only
                                 a maxval and no real value inside? */
                }

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
get_battery_max_value (const std::string &filename, const Ptr<t_chipfeature> &feature)
{
    std::string path;

#ifdef HAVE_SYSFS_ACPI
    path = xfce4::sprintf ("%s/%s/%s/%s", SYS_PATH, SYS_DIR_POWER, filename.c_str(), SYS_FILE_ENERGY_MAX);
#else
    path = xfce4::sprintf ("%s/%s/%s/%s", ACPI_PATH, ACPI_DIR_BATTERY, filename.c_str(), ACPI_FILE_BATTERY_INFO);
#endif
    FILE *file = fopen (path.c_str(), "r");
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
}


/* -------------------------------------------------------------------------- */
gint
read_fan_zone (const Ptr<t_chip> &chip)
{
    gint res_value = -1;

    if ((chdir (ACPI_PATH) == 0) && (chdir (ACPI_DIR_FAN) == 0))
    {
        struct dirent *entry;

        DIR *dir = opendir (".");
        while (dir && (entry = readdir (dir)))
        {
            if (strncmp(entry->d_name, ".", 1)==0)
                continue;

            std::string filename = xfce4::sprintf ("%s/%s/%s/%s", ACPI_PATH, ACPI_DIR_FAN, entry->d_name, ACPI_FILE_FAN);
            FILE *file = fopen (filename.c_str(), "r");
            if (file)
            {
                /* if (acpi_ignore_directory_entry (de))
                    continue; */

                auto feature = xfce4::make<t_chipfeature>();
                feature->color_orEmpty = "#0000B0";
                feature->address = chip->chip_features.size();
                feature->devicename = entry->d_name;
                feature->name = feature->devicename;
                feature->formatted_value = ""; /* Gonna refresh it in sensors_get_wrapper or some other functions */
                feature->raw_value = get_fan_zone_value (entry->d_name);
                feature->valid = true;
                feature->min_value = 0.0;
                feature->max_value = 2.0;
                feature->cls = STATE;

                chip->chip_features.push_back(feature);

                fclose(file);
            }

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
get_power_zone_value (const std::string &zone)
{
    gdouble res_value = 0;

    std::string filename = xfce4::sprintf ("%s/%s/%s/%s", SYS_PATH, SYS_DIR_POWER, zone.c_str(), SYS_FILE_POWER);

    FILE *file = fopen (filename.c_str(), "r");
    if (file) {
        gchar buffer[1024];
        if (fgets (buffer, 1024, file)!=NULL)
        {
            cut_newline (buffer);
            res_value = strtod (buffer, NULL) / 1000000.0;
        }
        fclose (file);
    }

    return res_value;
}


/* -------------------------------------------------------------------------- */
gdouble
get_voltage_zone_value (const std::string &zone)
{
    gdouble res_value = 0.0;

    std::string filename = xfce4::sprintf ("%s/%s/%s/%s", SYS_PATH, SYS_DIR_POWER, zone.c_str(), SYS_FILE_VOLTAGE);

    FILE *file = fopen (filename.c_str(), "r");
    if (file) {
        gchar buffer[1024];
        if (fgets (buffer, 1024, file)!=NULL)
        {
            cut_newline (buffer);
            res_value = strtod (buffer, NULL) / 1e6;
        }
        fclose (file);
    }

    return res_value;
}


/* -------------------------------------------------------------------------- */
gint
read_power_zone (const Ptr<t_chip> &chip)
{
    gint res_value = -1;

    if ((chdir (SYS_PATH) == 0) && (chdir (SYS_DIR_POWER) == 0)) {
        DIR *dir = opendir (".");
        struct dirent *entry;
        while (dir && (entry = readdir (dir)))
        {
            if (strncmp(entry->d_name, "BAT", 3)==0)
            { /* have a battery subdirectory */

                std::string filename = xfce4::sprintf ("%s/%s/%s/%s", SYS_PATH, SYS_DIR_POWER, entry->d_name, SYS_FILE_POWER);
                FILE *file = fopen (filename.c_str(), "r");
                if (file) {
                    auto feature = xfce4::make<t_chipfeature>();
                    feature->color_orEmpty = "#00B0B0";
                    feature->address = chip->chip_features.size();
                    feature->devicename = entry->d_name;
                    // You might want to format this with a hyphen and without spacing, or with a dash; the result might be BAT1–Power or whatever fits your language most. Spaces allow line breaks over the tachometers.
                    feature->name = xfce4::sprintf (_("%s - %s"),
                                                    // Power with unit Watts, not Energy with Joules or kWh
                                                    entry->d_name, _("Power"));
                    feature->formatted_value = "";
                    feature->raw_value = get_power_zone_value (entry->d_name);
                    feature->valid = true;
                    feature->min_value = 0.0;
                    feature->max_value = 60.0; // a T440s charges with roughly 25 Watts
                    feature->cls = POWER;

                    chip->chip_features.push_back(feature);

                    fclose (file);
                }
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
read_voltage_zone (const Ptr<t_chip> &chip)
{
    gint res_value = -1;

    if ((chdir (SYS_PATH) == 0) && (chdir (SYS_DIR_POWER) == 0)) {
        DIR *dir = opendir (".");
        struct dirent *entry;
        while (dir && (entry = readdir (dir)))
        {
            if (strncmp(entry->d_name, "BAT", 3)==0)
            { /* have a battery subdirectory */

                std::string filename = xfce4::sprintf ("%s/%s/%s/%s", SYS_PATH, SYS_DIR_POWER, entry->d_name, SYS_FILE_VOLTAGE);
                FILE *file = fopen (filename.c_str(), "r");
                if (file) {
                    auto feature = xfce4::make<t_chipfeature>();
                    feature->color_orEmpty = "#00B0B0";
                    feature->address = chip->chip_features.size();
                    feature->devicename = entry->d_name;
                    // You might want to format this with a hyphen and without spacing, or with a dash; the result might be BAT1–Voltage or whatever fits your language most. Spaces allow line breaks over the tachometers.
                    feature->name = xfce4::sprintf (_("%s - %s"), entry->d_name, _("Voltage"));
                    feature->formatted_value = "";
                    feature->raw_value = get_voltage_zone_value(entry->d_name);
                    feature->valid = true;
                    auto zone = xfce4::sprintf ("%s/%s/%s/%s", SYS_PATH, SYS_DIR_POWER, entry->d_name, SYS_FILE_VOLTAGE_MIN);
                    std::string min_voltage = get_acpi_value (zone);
                    feature->min_value = feature->raw_value;
                    if (!min_voltage.empty())
                    {
                        feature->min_value = strtod (min_voltage.c_str(), NULL) / 1000000.0;
                    }
                    feature->max_value = feature->raw_value; // a T440s charges with roughly 25 Watts
                    feature->cls = VOLTAGE;

                    chip->chip_features.push_back(feature);

                    fclose (file);
                }
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
initialize_ACPI (std::vector<Ptr<t_chip>> &chips)
{
    auto chip = xfce4::make<t_chip>();

    chip->name = _("ACPI"); /* to be displayed */

    std::string acpi_info = get_acpi_info();
    chip->description = xfce4::sprintf (_("ACPI v%s zones"), acpi_info.c_str());
    chip->sensorId = "ACPI"; /* used internally */

    chip->type = ACPI;

    sensors_chip_name *chip_name = g_new0 (sensors_chip_name, 1);
    g_return_val_if_fail (chip_name != NULL, -1);

    chip_name->prefix = g_strdup(_("ACPI"));

    chip->chip_name = chip_name;

    read_battery_zone (chip);
    read_thermal_zone (chip);
    read_fan_zone (chip);

#ifdef HAVE_SYSFS_ACPI
    read_power_zone (chip);
    read_voltage_zone (chip);
#endif

    chips.push_back(chip);

    return 4;
}


/* -------------------------------------------------------------------------- */
void
refresh_acpi (const Ptr<t_chipfeature> &feature)
{
    switch (feature->cls) {
        case TEMPERATURE: {
#ifdef HAVE_SYSFS_ACPI
            auto zone = xfce4::sprintf ("%s/%s/%s/%s", SYS_PATH, SYS_DIR_THERMAL, feature->devicename.c_str(), SYS_FILE_THERMAL);
            FILE *file = fopen(zone.c_str(), "r");
            if (file)
            {
                gchar buffer[1024];
                if (fgets (buffer, sizeof(buffer), file)) /* automatically null-terminated */
                {
                    cut_newline(buffer);
                    feature->raw_value = strtod(buffer, NULL) / 1000.0;
                }
                fclose (file);
            }
#else
            auto zone = xfce4::sprintf ("%s/%s", ACPI_DIR_THERMAL, feature->devicename.c_str());
            feature->raw_value = get_acpi_zone_value (zone, ACPI_FILE_THERMAL);
#endif
            break;
        }

        case ENERGY:
            feature->raw_value = get_battery_zone_value (feature->devicename);
            break;

        case POWER:
            feature->raw_value = get_power_zone_value (feature->devicename);
            break;

        case VOLTAGE:
            feature->raw_value = get_voltage_zone_value (feature->devicename);
            break;

        case STATE: {
            auto filename = xfce4::sprintf ("%s/%s/%s/state", ACPI_PATH, ACPI_DIR_FAN, feature->devicename.c_str());

            std::string state = get_acpi_value(filename);
            if (state.empty())
            {
                DBG("Could not determine fan state.");
                feature->raw_value = 0.0;
            }
            else
            {
                feature->raw_value = strncmp(state.c_str(), "on", 2)==0 ? 1.0 : 0.0;
            }
            break;
        }

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
std::string
get_acpi_info ()
{
    auto filename = xfce4::sprintf ("%s/%s", ACPI_PATH, ACPI_INFO);

    std::string version = get_acpi_value (filename);

    if (version.empty())
    {
        filename = xfce4::sprintf ("%s/%s_", ACPI_PATH, ACPI_INFO);
        version = get_acpi_value (filename);

        if (version.empty())
            version = get_acpi_value ("/sys/module/acpi/parameters/acpica_str_version");
    }

    version = xfce4::trim (version);

    if (version.empty())
        version = _("<Unknown>");

    return version;
}


/* -------------------------------------------------------------------------- */
/**
 * Note that zone will have to consist of two paths, e.g.
 * thermal_zone and THRM.
 */
gdouble
get_acpi_zone_value (const std::string &zone, const std::string &filename)
{
    auto zone_filename = xfce4::sprintf ("%s/%s/%s", ACPI_PATH, zone.c_str(), filename.c_str());
    std::string value = get_acpi_value (zone_filename);

    /* Return it as a double */
    if (!value.empty())
        return strtod (value.c_str(), NULL);
    else
        return 0;
}


/* -------------------------------------------------------------------------- */
std::string
get_acpi_value (const std::string &filename)
{
    std::string result;

    FILE *file = fopen (filename.c_str(), "r");
    if (file)
    {
        gchar buffer[1024];
        if (fgets (buffer, sizeof(buffer), file)) /* appends null-byte character at end */
        {
            gchar *valueinstring = strip_key_colon_spaces (buffer);
            g_assert(valueinstring!=NULL); /* points to beginning of buffer at least */

            result = valueinstring;
        }

        fclose (file);
    }

    /* Have read the data */
    return result;
}


/* -------------------------------------------------------------------------- */
void
free_acpi_chip (t_chip *chip)
{
    if (chip->chip_name)
    {
        g_free (chip->chip_name->path);
        g_free (chip->chip_name->prefix);
        chip->chip_name->path = NULL;
        chip->chip_name->prefix = NULL;
    }
}
