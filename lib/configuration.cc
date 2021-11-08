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

/* The fixes file has to be included before any other #include directives */
#include "xfce4++/util/fixes.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libxfce4panel/xfce-panel-plugin.h>
#include <libxfce4util/libxfce4util.h>
#include <stdlib.h>
#include <unistd.h>
#include "xfce4++/util.h"

/* Package includes */
#include <configuration.h>
#include <sensors-interface.h>
#include <tacho.h>
#include <types.h>


/* -------------------------------------------------------------------------- */
gint
get_Id_from_address (gint chip_number, gint addr_chipfeature, const Ptr<t_sensors> &sensors)
{
    gint result = -1;

    auto chip = sensors->chips[chip_number];
    for (size_t idx_feature = 0; idx_feature < chip->chip_features.size(); idx_feature++) {
        Ptr<t_chipfeature> feature = chip->chip_features[idx_feature];
        DBG("address: %d", feature->address);
        if (addr_chipfeature == feature->address) {
            result = idx_feature;
            break;
        }
    }

    return result;
}


/* -------------------------------------------------------------------------- */
void
sensors_write_config (XfcePanelPlugin *plugin, const Ptr<const t_sensors> &sensors)
{
    const std::string &file = sensors->plugin_config_file;
    if (!file.empty())
    {
        unlink (file.c_str());

        Ptr0<xfce4::Rc> rc = xfce4::Rc::simple_open (file, FALSE);
        if (rc)
        {
            rc->set_group ("General");

            rc->write_bool_entry ("Show_Title", sensors->show_title);
            rc->write_bool_entry ("Show_Labels", sensors->show_labels);
            rc->write_bool_entry ("Show_Colored_Bars", !sensors->automatic_bar_colors);
            rc->write_bool_entry ("Exec_Command", sensors->exec_command);
            rc->write_bool_entry ("Show_Units", sensors->show_units);
            rc->write_bool_entry ("Small_Spacings", sensors->show_smallspacings);
            rc->write_bool_entry ("Cover_All_Panel_Rows", sensors->cover_panel_rows);
            rc->write_bool_entry ("Suppress_Hddtemp_Message", sensors->suppressmessage);
            rc->write_bool_entry ("Suppress_Tooltip", sensors->suppresstooltip);

            rc->write_int_entry ("Use_Bar_UI", sensors->display_values_type);
            rc->write_int_entry ("Scale", sensors->scale);
            rc->write_int_entry ("val_fontsize", sensors->val_fontsize);
            rc->write_int_entry ("Lines_Size", sensors->lines_size);
            rc->write_int_entry ("Update_Interval", sensors->sensors_refresh_time);
            rc->write_int_entry ("Number_Chips", sensors->chips.size());
            rc->write_int_entry ("Preferred_Width", sensors->preferred_width);
            rc->write_int_entry ("Preferred_Height", sensors->preferred_height);

            rc->write_entry ("str_fontsize", sensors->str_fontsize);
            rc->write_entry ("Command_Name", sensors->command_name);
            rc->write_entry ("Tachos_ColorValue", xfce4::sprintf ("%.2f", sensors->val_tachos_color));
            rc->write_entry ("Tachos_Alpha", xfce4::sprintf ("%.2f", sensors->val_tachos_alpha));
            if (!font.empty())
                rc->write_entry ("Font", font); // the font for the tachometers exported from tacho.h

            for (size_t idx_chip = 0; idx_chip < sensors->chips.size(); idx_chip++)
            {
                auto chip = sensors->chips[idx_chip];

                const auto str_chip = xfce4::sprintf ("Chip%zu", idx_chip);
                rc->set_group (str_chip);

                rc->write_entry ("Name", chip->sensorId);
                rc->write_int_entry ("Number", idx_chip);

                for (size_t idx_feature = 0; idx_feature < chip->chip_features.size(); idx_feature++) {
                    Ptr<t_chipfeature> feature = chip->chip_features[idx_feature];

                    if (feature->show)
                    {
                        rc->set_group (xfce4::sprintf ("%s_Feature%zu", str_chip.c_str(), idx_feature));

                        /* only use this if no hddtemp sensor */
                        /* or do only use this , if it is an lmsensors device. whatever. */
                        if (chip->sensorId != _("Hard disks")) /* chip->name? */
                             rc->write_int_entry ("Address", idx_feature);
                        else
                             rc->write_entry ("DeviceName", feature->devicename);

                        rc->write_entry ("Name", feature->name);

                        if (!feature->color_orEmpty.empty())
                            rc->write_entry ("Color", feature->color_orEmpty);
                        else
                            rc->delete_entry ("Color", FALSE);

                        rc->write_bool_entry ("Show", feature->show);

                        rc->write_entry ("Min", xfce4::sprintf ("%.2f", feature->min_value));
                        rc->write_entry ("Max", xfce4::sprintf ("%.2f", feature->max_value));
                    }
                }
            }

            rc->close();
        }
    }
}


/* -------------------------------------------------------------------------- */
void
sensors_read_general_config (const Ptr0<xfce4::Rc> &rc, const Ptr<t_sensors> &sensors)
{
    g_return_if_fail(rc != nullptr);

    if (rc->has_group ("General"))
    {
        rc->set_group ("General");

        sensors->show_title = rc->read_bool_entry ("Show_Title", false);
        sensors->show_labels = rc->read_bool_entry ("Show_Labels", true);
        sensors->automatic_bar_colors = !rc->read_bool_entry ("Show_Colored_Bars", false);

        gint display_values_type = rc->read_int_entry ("Use_Bar_UI", -1);
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

        gint scale = rc->read_int_entry ("Scale", -1);
        switch (scale)
        {
            case CELSIUS:
            case FAHRENHEIT:
                sensors->scale = (t_tempscale) scale;
                break;
            default:
                sensors->scale = CELSIUS;
        }

        Ptr0<std::string> str_value;
        if ((str_value = rc->read_entry ("str_fontsize", NULL)) && !str_value->empty())
            sensors->str_fontsize = *str_value;

        if ((str_value = rc->read_entry ("Font", NULL)) && !str_value->empty())
            font = *str_value; // in tacho.h for the tachometers
        else
            font = default_font;

        sensors->cover_panel_rows   = rc->read_bool_entry ("Cover_All_Panel_Rows", FALSE);
        sensors->exec_command       = rc->read_bool_entry ("Exec_Command", TRUE);
        sensors->show_units         = rc->read_bool_entry ("Show_Units", TRUE);
        sensors->show_smallspacings = rc->read_bool_entry ("Small_Spacings", FALSE);
        sensors->suppresstooltip    = rc->read_bool_entry ("Suppress_Tooltip", FALSE);

        sensors->val_fontsize         = rc->read_int_entry ("val_fontsize", 2);
        sensors->lines_size           = rc->read_int_entry ("Lines_Size", 3);
        sensors->sensors_refresh_time = rc->read_int_entry ("Update_Interval", 60);
        sensors->preferred_width      = rc->read_int_entry ("Preferred_Width", 400);
        sensors->preferred_height     = rc->read_int_entry ("Preferred_Height", 400);

        if ((str_value = rc->read_entry ("Command_Name", NULL)) && !str_value->empty())
            sensors->command_name = *str_value;

        if (!sensors->suppressmessage)
            sensors->suppressmessage = rc->read_bool_entry ("Suppress_Hddtemp_Message", FALSE);

        if ((str_value = rc->read_entry ("Tachos_ColorValue", NULL)) && !str_value->empty())
            sensors->val_tachos_color = atof (str_value->c_str());

        if ((str_value = rc->read_entry ("Tachos_Alpha", NULL)) && !str_value->empty())
            sensors->val_tachos_alpha = atof (str_value->c_str());

        //num_chips = xfce_rc_read_int_entry (ptr_xfceresources, "Number_Chips", 0);
        /* or could use 1 or the always existent dummy entry */
    }
}


/* -------------------------------------------------------------------------- */
void
sensors_read_preliminary_config (XfcePanelPlugin *plugin, const Ptr<t_sensors> &sensors)
{
    if (plugin)
    {
        if (!sensors->plugin_config_file.empty())
        {
            auto rc = xfce4::Rc::simple_open (sensors->plugin_config_file, TRUE);
            if (rc)
            {
                if (rc->has_group ("General")) {
                    rc->set_group ("General");
                    sensors->suppressmessage = rc->read_bool_entry ("Suppress_Hddtemp_Message", FALSE);
                }
                rc->close ();
            }
        }
    }
}


/* -------------------------------------------------------------------------- */
// TODO: Modify to store chipname as indicator and access features by acpitz-1_Feature0 etc.
//       This will require differently storing the stuff as well.
//       Targeted for 1.4.x release
void
sensors_read_config (XfcePanelPlugin *plugin, const Ptr<t_sensors> &sensors)
{
    g_return_if_fail(plugin!=NULL);

    if (sensors->plugin_config_file.empty())
        return;

    Ptr0<xfce4::Rc> rc = xfce4::Rc::simple_open (sensors->plugin_config_file, TRUE);
    if (!rc)
        return;

    sensors_read_general_config (rc, sensors);

    for (size_t idx_chip = 0; idx_chip < sensors->chips.size(); idx_chip++)
    {
        auto str_chip = xfce4::sprintf ("Chip%zu", idx_chip);
        if (rc->has_group (str_chip))
        {
            Ptr0<std::string> str_value;
            rc->set_group (str_chip);
            if ((str_value = rc->read_entry ("Name", NULL)) && !str_value->empty())
            {
                const std::string sensor_name = *str_value;
                const gint num_sensorchip = rc->read_int_entry ("Number", 0);
                if (num_sensorchip >= 0 && guint(num_sensorchip) < sensors->chips.size())
                {
                    Ptr0<t_chip> chip;

                    /* now featuring enhanced string comparison */
                    size_t idx_chiptmp = 0;
                    do {
                      chip = sensors->chips[idx_chiptmp++];
                      if (idx_chiptmp == sensors->chips.size())
                          break;
                    }
                    while (chip && chip->sensorId != sensor_name);

                    if (chip && chip->sensorId == sensor_name)
                    {
                        for (size_t idx_feature = 0; idx_feature < chip->chip_features.size(); idx_feature++)
                        {
                            Ptr<t_chipfeature> feature = chip->chip_features[idx_feature];

                            auto str_feature = xfce4::sprintf ("%s_Feature%zu", str_chip.c_str(), idx_feature);
                            if (rc->has_group (str_feature))
                            {
                                rc->set_group (str_feature);

                                if ((str_value = rc->read_entry ("DeviceName", NULL)) && !str_value->empty())
                                    feature->devicename = *str_value;

                                if ((str_value = rc->read_entry ("Name", NULL)) && !str_value->empty())
                                    feature->name = *str_value;

                                if ((str_value = rc->read_entry ("Color", NULL)) && !str_value->empty())
                                    feature->color_orEmpty = *str_value;
                                else
                                    feature->color_orEmpty = "";

                                feature->show = rc->read_bool_entry ("Show", FALSE);

                                if ((str_value = rc->read_entry ("Min", NULL)) && !str_value->empty())
                                    feature->min_value = atof (str_value->c_str());

                                if ((str_value = rc->read_entry ("Max", NULL)) && !str_value->empty())
                                    feature->max_value = atof (str_value->c_str());
                            }
                        }
                    }
                }
            }
        }
    }

    rc->close ();

    if (!sensors->exec_command)
        g_signal_handler_block (G_OBJECT(sensors->eventbox), sensors->doubleclick_id);
}
