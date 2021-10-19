/* configuration.cc
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

#include <libxfce4panel/xfce-panel-plugin.h>
#include <libxfce4util/libxfce4util.h>
#include <stdlib.h>
#include <unistd.h>

/* Package includes */
#include <configuration.h>
#include <sensors-interface.h>
#include <tacho.h>
#include <types.h>


/* -------------------------------------------------------------------------- */
gint
get_Id_from_address (gint chip_number, gint addr_chipfeature, t_sensors *sensors)
{
    gint result = -1;

    g_return_val_if_fail (sensors!=NULL, result);

    auto chip = (t_chip*) g_ptr_array_index (sensors->chips, chip_number);

    if (chip)
    {
        for (gint idx_feature=0; idx_feature<chip->num_features; idx_feature++) {
            auto feature = (t_chipfeature*) g_ptr_array_index(chip->chip_features, idx_feature);
            if (feature)
            {
                DBG("address: %d", feature->address);
                if (addr_chipfeature == feature->address) {
                    result = idx_feature;
                    break;
                }
            }
        }
    }

    return result;
}


/* -------------------------------------------------------------------------- */
void
sensors_write_config (XfcePanelPlugin *plugin, const t_sensors *sensors)
{
    g_return_if_fail (sensors != NULL);

    const std::string &file = sensors->plugin_config_file;
    if (!file.empty())
    {
        unlink (file.c_str());

        XfceRc *rc = xfce_rc_simple_open (file.c_str(), FALSE);
        if (rc)
        {
            gchar *tmp;

            xfce_rc_set_group (rc, "General");

            xfce_rc_write_bool_entry (rc, "Show_Title", sensors->show_title);

            xfce_rc_write_bool_entry (rc, "Show_Labels", sensors->show_labels);

            xfce_rc_write_int_entry (rc, "Use_Bar_UI", sensors->display_values_type);

            xfce_rc_write_bool_entry (rc, "Show_Colored_Bars", !sensors->automatic_bar_colors);

            xfce_rc_write_int_entry (rc, "Scale", sensors->scale);

            xfce_rc_write_entry (rc, "str_fontsize", sensors->str_fontsize.c_str());

            xfce_rc_write_int_entry (rc, "val_fontsize", sensors->val_fontsize);

            if (!font.empty())
                xfce_rc_write_entry (rc, "Font", font.c_str()); // the font for the tachometers exported from tacho.h

            xfce_rc_write_int_entry (rc, "Lines_Size", sensors->lines_size);

            xfce_rc_write_bool_entry (rc, "Cover_All_Panel_Rows", sensors->cover_panel_rows);

            xfce_rc_write_int_entry (rc, "Update_Interval", sensors->sensors_refresh_time);

            xfce_rc_write_bool_entry (rc, "Exec_Command", sensors->exec_command);

            xfce_rc_write_bool_entry (rc, "Show_Units", sensors->show_units);

            xfce_rc_write_bool_entry(rc, "Small_Spacings", sensors->show_smallspacings);

            xfce_rc_write_entry (rc, "Command_Name", sensors->command_name.c_str());

            xfce_rc_write_int_entry (rc, "Number_Chips", sensors->num_sensorchips);

            xfce_rc_write_bool_entry (rc, "Suppress_Hddtemp_Message", sensors->suppressmessage);

            xfce_rc_write_bool_entry (rc, "Suppress_Tooltip", sensors->suppresstooltip);

            xfce_rc_write_int_entry (rc, "Preferred_Width", sensors->preferred_width);
            xfce_rc_write_int_entry (rc, "Preferred_Height", sensors->preferred_height);

            tmp = g_strdup_printf("%.2f", sensors->val_tachos_color);
            xfce_rc_write_entry (rc, "Tachos_ColorValue", tmp);
            g_free (tmp);

            tmp = g_strdup_printf("%.2f", sensors->val_tachos_alpha);
            xfce_rc_write_entry (rc, "Tachos_Alpha", tmp);
            g_free (tmp);

            for (gint idx_chip=0; idx_chip<sensors->num_sensorchips; idx_chip++)
            {
                auto chip = (t_chip*) g_ptr_array_index (sensors->chips, idx_chip);
                g_assert (chip!=NULL);

                gchar str_chip[8];
                g_snprintf (str_chip, sizeof (str_chip), "Chip%d", idx_chip);

                xfce_rc_set_group (rc, str_chip);

                xfce_rc_write_entry (rc, "Name", chip->sensorId.c_str());

                /* number of sensors is still limited */
                xfce_rc_write_int_entry (rc, "Number", idx_chip);

                for (gint idx_feature=0; idx_feature<chip->num_features; idx_feature++) {
                    auto feature = (t_chipfeature*) g_ptr_array_index(chip->chip_features, idx_feature);
                    g_assert (feature!=NULL);

                    if (feature->show)
                    {
                        gchar str_feature[20];
                        g_snprintf (str_feature, sizeof (str_feature), "%s_Feature%d", str_chip, idx_feature);

                        xfce_rc_set_group (rc, str_feature);

                        /* only use this if no hddtemp sensor */
                        /* or do only use this , if it is an lmsensors device. whatever. */
                        if (chip->sensorId == _("Hard disks")) /* chip->name? */
                             xfce_rc_write_int_entry (rc, "Address", idx_feature);
                        else
                             xfce_rc_write_entry (rc, "DeviceName", feature->devicename.c_str());

                        xfce_rc_write_entry (rc, "Name", feature->name.c_str());

                        if (!feature->color_orEmpty.empty())
                            xfce_rc_write_entry (rc, "Color", feature->color_orEmpty.c_str());
                        else
                            xfce_rc_delete_entry (rc, "Color", FALSE);

                        xfce_rc_write_bool_entry (rc, "Show", feature->show);

                        tmp = g_strdup_printf("%.2f", feature->min_value);
                        xfce_rc_write_entry (rc, "Min", tmp);
                        g_free (tmp);

                        tmp = g_strdup_printf("%.2f", feature->max_value);
                        xfce_rc_write_entry (rc, "Max", tmp);
                        g_free (tmp);
                    }
                }
            }

            xfce_rc_close (rc);
        }
    }
}


/* -------------------------------------------------------------------------- */
void
sensors_read_general_config (XfceRc *rc, t_sensors *sensors)
{
    g_return_if_fail(rc!=NULL);
    g_return_if_fail(sensors!=NULL);

    if (xfce_rc_has_group (rc, "General") )
    {
        xfce_rc_set_group (rc, "General");

        sensors->show_title = xfce_rc_read_bool_entry (rc, "Show_Title", TRUE);

        sensors->show_labels = xfce_rc_read_bool_entry (rc, "Show_Labels", TRUE);

        gint display_values_type = xfce_rc_read_int_entry (rc, "Use_Bar_UI", -1);
        switch (display_values_type)
        {
            case DISPLAY_BARS:
            case DISPLAY_TACHO:
            case DISPLAY_TEXT:
                sensors->display_values_type = (e_displaystyles) display_values_type;
                break;
            default:
                sensors->display_values_type = DISPLAY_TEXT;
        }

        sensors->automatic_bar_colors = !xfce_rc_read_bool_entry (rc, "Show_Colored_Bars", FALSE);

        gint scale = xfce_rc_read_int_entry (rc, "Scale", -1);
        switch (scale)
        {
            case CELSIUS:
            case FAHRENHEIT:
                sensors->scale = (t_tempscale) scale;
                break;
            default:
                sensors->scale = CELSIUS;
        }

        const gchar *str_value;
        if ((str_value = xfce_rc_read_entry (rc, "str_fontsize", NULL)) && *str_value) {
            sensors->str_fontsize = str_value;
        }

        if ((str_value = xfce_rc_read_entry (rc, "Font", NULL)) && *str_value)
            font = str_value; // in tacho.h for the tachometers
        else
            font = default_font;

        sensors->val_fontsize = xfce_rc_read_int_entry (rc, "val_fontsize", 2);

        sensors->lines_size = xfce_rc_read_int_entry (rc, "Lines_Size", 3);

        sensors->cover_panel_rows = xfce_rc_read_bool_entry (rc, "Cover_All_Panel_Rows", FALSE);

        sensors->sensors_refresh_time = xfce_rc_read_int_entry (rc, "Update_Interval", 60);

        sensors->exec_command = xfce_rc_read_bool_entry (rc, "Exec_Command", TRUE);

        sensors->show_units= xfce_rc_read_bool_entry (rc, "Show_Units", TRUE);

        sensors->show_smallspacings= xfce_rc_read_bool_entry (rc, "Small_Spacings", FALSE);

        if ((str_value = xfce_rc_read_entry (rc, "Command_Name", NULL)) && *str_value) {
            sensors->command_name = str_value;
        }

        if (!sensors->suppressmessage)
            sensors->suppressmessage = xfce_rc_read_bool_entry (rc, "Suppress_Hddtemp_Message", FALSE);

        sensors->suppresstooltip = xfce_rc_read_bool_entry (rc, "Suppress_Tooltip", FALSE);

        sensors->preferred_width = xfce_rc_read_int_entry (rc, "Preferred_Width", 400);
        sensors->preferred_height = xfce_rc_read_int_entry (rc, "Preferred_Height", 400);

        if ((str_value = xfce_rc_read_entry (rc, "Tachos_ColorValue", NULL)) && *str_value)
            sensors->val_tachos_color = atof (str_value);

        if ((str_value = xfce_rc_read_entry (rc, "Tachos_Alpha", NULL)) && *str_value)
            sensors->val_tachos_alpha = atof (str_value);

        //num_chips = xfce_rc_read_int_entry (ptr_xfceresources, "Number_Chips", 0);
        /* or could use 1 or the always existent dummy entry */
    }
}


/* -------------------------------------------------------------------------- */
void
sensors_read_preliminary_config (XfcePanelPlugin *plugin, t_sensors *sensors)
{
    if (plugin)
    {
        g_return_if_fail (sensors != NULL);
        if (!sensors->plugin_config_file.empty())
        {
            XfceRc *rc = xfce_rc_simple_open (sensors->plugin_config_file.c_str(), TRUE);
            if (rc)
            {
                if (xfce_rc_has_group (rc, "General")) {
                    xfce_rc_set_group (rc, "General");
                    sensors->suppressmessage = xfce_rc_read_bool_entry (rc, "Suppress_Hddtemp_Message", FALSE);
                }
                xfce_rc_close (rc);
            }
        }
    }
}


/* -------------------------------------------------------------------------- */
// TODO: Modify to store chipname as indicator and access features by acpitz-1_Feature0 etc.
//       This will require differently storing the stuff as well.
//       Targeted for 1.4.x release
void
sensors_read_config (XfcePanelPlugin *plugin, t_sensors *sensors)
{
    g_return_if_fail(plugin!=NULL);
    g_return_if_fail(sensors!=NULL);

    if (sensors->plugin_config_file.empty())
        return;

    XfceRc *rc = xfce_rc_simple_open (sensors->plugin_config_file.c_str(), TRUE);
    if (!rc)
        return;

    sensors_read_general_config (rc, sensors);

    for (gint idx_chip = 0; idx_chip < sensors->num_sensorchips; idx_chip++)
    {
        gchar str_chip[8];
        g_snprintf (str_chip, sizeof (str_chip), "Chip%d", idx_chip);
        if (xfce_rc_has_group (rc, str_chip))
        {
            const gchar *str_value;
            xfce_rc_set_group (rc, str_chip);
            if ((str_value = xfce_rc_read_entry (rc, "Name", NULL)) && *str_value)
            {
                const std::string sensor_name = str_value;
                const gint num_sensorchip = xfce_rc_read_int_entry (rc, "Number", 0);
                if (num_sensorchip < sensors->num_sensorchips)
                {
                    t_chip *chip;

                    /* now featuring enhanced string comparison */
                    gint idx_chiptmp = 0;
                    do {
                      chip = (t_chip *) g_ptr_array_index (sensors->chips, idx_chiptmp++);
                      if (!chip || idx_chiptmp == sensors->num_sensorchips)
                          break;
                    }
                    while (chip && chip->sensorId != sensor_name);

                    if (chip && chip->sensorId == sensor_name)
                    {
                        for (gint idx_feature = 0; idx_feature < chip->num_features; idx_feature++)
                        {
                            gchar str_feature[20];

                            auto feature = (t_chipfeature*) g_ptr_array_index (chip->chip_features, idx_feature);
                            g_assert (feature!=NULL);

                            g_snprintf (str_feature, sizeof (str_feature), "%s_Feature%d", str_chip, idx_feature);

                            if (xfce_rc_has_group (rc, str_feature))
                            {
                                xfce_rc_set_group (rc, str_feature);

                                if ((str_value = xfce_rc_read_entry (rc, "DeviceName", NULL)) && *str_value)
                                {
                                    feature->devicename = str_value;
                                }

                                if ((str_value = xfce_rc_read_entry (rc, "Name", NULL)) && *str_value)
                                {
                                    feature->name = str_value;
                                }

                                if ((str_value = xfce_rc_read_entry (rc, "Color", NULL)) && *str_value)
                                    feature->color_orEmpty = str_value;
                                else
                                    feature->color_orEmpty = "";

                                feature->show = xfce_rc_read_bool_entry (rc, "Show", FALSE);

                                if ((str_value = xfce_rc_read_entry (rc, "Min", NULL)) && *str_value)
                                    feature->min_value = atof (str_value);

                                if ((str_value = xfce_rc_read_entry (rc, "Max", NULL)) && *str_value)
                                    feature->max_value = atof (str_value);
                            }
                        }
                    }
                }
            }
        }
    }

    xfce_rc_close (rc);

    if (!sensors->exec_command)
        g_signal_handler_block ( G_OBJECT(sensors->eventbox), sensors->doubleclick_id);
}
