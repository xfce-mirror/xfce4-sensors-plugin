/* File: configuration.c
 *
 *  Copyright 2004-2017 Fabian Nowak (timystery@arcor.de)
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/* Global includes */
#include <stdlib.h>
#include <unistd.h>

/* Xfce includes */
#include <libxfce4util/libxfce4util.h>
#include <libxfce4panel/xfce-panel-plugin.h>

/* Package includes */
#include <configuration.h>
#include <tacho.h>
#include <sensors-interface.h>
#include <types.h>


/* -------------------------------------------------------------------------- */
gint
get_Id_from_address (gint idx_chip, gint addr_chipfeature, t_sensors *ptr_sensors)
{
    gint idx_feature, result = -1;
    t_chip *ptr_chip = NULL;
    t_chipfeature *ptr_chipfeature = NULL;

    TRACE ("enters get_Id_from_address");

    g_return_val_if_fail(ptr_sensors!=NULL, result);

    ptr_chip = (t_chip *) g_ptr_array_index (ptr_sensors->chips, idx_chip);

    if (ptr_chip)
    {
        for (idx_feature=0; idx_feature<ptr_chip->num_features; idx_feature++) {
            ptr_chipfeature = g_ptr_array_index(ptr_chip->chip_features, idx_feature);
            if (ptr_chipfeature)
            {
                DBG("address: %d", ptr_chipfeature->address);
                if (addr_chipfeature == ptr_chipfeature->address) {
                    result = idx_feature;
                    break;
                }
            }
        }
    }

    TRACE ("leaves get_Id_from_address with %d", result);
    return result;
}


/* -------------------------------------------------------------------------- */
void
sensors_write_config (XfcePanelPlugin *plugin,t_sensors *ptr_sensors)
{
    XfceRc *ptr_xfcerc;
    gchar *str_file, *str_tmp, str_chip[8], str_feature[20];
    gint idx_chips, idx_features;
    t_chip *ptr_chip;
    t_chipfeature *ptr_chipfeature;

    TRACE ("enters sensors_write_config: 0x%llX.", (unsigned long long int) ptr_sensors);

    g_return_if_fail(ptr_sensors!=NULL);

    if ( ! (str_file = ptr_sensors->plugin_config_file) ) {
        TRACE ("leaves sensors_write_config: No file location specified.");
    }
    else
    {
        unlink (str_file);

        ptr_xfcerc = xfce_rc_simple_open (str_file, FALSE);

        if (!ptr_xfcerc) {
            TRACE ("leaves sensors_write_config: No rc file opened");
        }
        else
        {
            xfce_rc_set_group (ptr_xfcerc, "General");

            xfce_rc_write_bool_entry (ptr_xfcerc, "Show_Title", ptr_sensors->show_title);

            xfce_rc_write_bool_entry (ptr_xfcerc, "Show_Labels", ptr_sensors->show_labels);

            xfce_rc_write_int_entry (ptr_xfcerc, "Use_Bar_UI", ptr_sensors->display_values_type);

            xfce_rc_write_bool_entry (ptr_xfcerc, "Show_Colored_Bars", ptr_sensors->show_colored_bars);

            xfce_rc_write_int_entry (ptr_xfcerc, "Scale", ptr_sensors->scale);

            xfce_rc_write_entry (ptr_xfcerc, "str_fontsize", ptr_sensors->str_fontsize);

            xfce_rc_write_int_entry (ptr_xfcerc, "val_fontsize",
                                        ptr_sensors->val_fontsize);

            if (font)
                xfce_rc_write_entry (ptr_xfcerc, "Font", font); // the font for the tachometers exported from tacho.h

            xfce_rc_write_int_entry (ptr_xfcerc, "Lines_Size", ptr_sensors->lines_size);

            xfce_rc_write_bool_entry (ptr_xfcerc, "Cover_All_Panel_Rows", ptr_sensors->cover_panel_rows);

            xfce_rc_write_int_entry (ptr_xfcerc, "Update_Interval", ptr_sensors->sensors_refresh_time);

            xfce_rc_write_bool_entry (ptr_xfcerc, "Exec_Command", ptr_sensors->exec_command);

            xfce_rc_write_bool_entry (ptr_xfcerc, "Show_Units", ptr_sensors->show_units);

            xfce_rc_write_bool_entry(ptr_xfcerc, "Small_Spacings", ptr_sensors->show_smallspacings);

            xfce_rc_write_entry (ptr_xfcerc, "Command_Name", ptr_sensors->command_name);

            xfce_rc_write_int_entry (ptr_xfcerc, "Number_Chips", ptr_sensors->num_sensorchips);

            xfce_rc_write_bool_entry (ptr_xfcerc, "Suppress_Hddtemp_Message", ptr_sensors->suppressmessage);

            xfce_rc_write_bool_entry (ptr_xfcerc, "Suppress_Tooltip", ptr_sensors->suppresstooltip);

            xfce_rc_write_int_entry (ptr_xfcerc, "Preferred_Width", ptr_sensors->preferred_width);
            xfce_rc_write_int_entry (ptr_xfcerc, "Preferred_Height", ptr_sensors->preferred_height);

            for (idx_chips=0; idx_chips<ptr_sensors->num_sensorchips; idx_chips++) {

                ptr_chip = (t_chip *) g_ptr_array_index(ptr_sensors->chips, idx_chips);
                g_assert (ptr_chip!=NULL);

                g_snprintf (str_chip, 8, "Chip%d", idx_chips);

                xfce_rc_set_group (ptr_xfcerc, str_chip);

                xfce_rc_write_entry (ptr_xfcerc, "Name", ptr_chip->sensorId);

                /* number of sensors is still limited */
                xfce_rc_write_int_entry (ptr_xfcerc, "Number", idx_chips);

                for (idx_features=0; idx_features<ptr_chip->num_features; idx_features++) {
                    ptr_chipfeature = g_ptr_array_index(ptr_chip->chip_features, idx_features);
                    g_assert (ptr_chipfeature!=NULL);

                    if (ptr_chipfeature->show) {

                       g_snprintf (str_feature, 20, "%s_Feature%d", str_chip, idx_features);

                       xfce_rc_set_group (ptr_xfcerc, str_feature);

                       /* only use this if no hddtemp sensor */
                       /* or do only use this , if it is an lmsensors device. whatever. */
                       if ( strcmp(ptr_chip->sensorId, _("Hard disks")) != 0 ) /* chip->name? */
                            xfce_rc_write_int_entry (ptr_xfcerc, "Address", idx_features);
                        else
                            xfce_rc_write_entry (ptr_xfcerc, "DeviceName", ptr_chipfeature->devicename);

                       xfce_rc_write_entry (ptr_xfcerc, "Name", ptr_chipfeature->name);

                       xfce_rc_write_entry (ptr_xfcerc, "Color", ptr_chipfeature->color);

                       xfce_rc_write_bool_entry (ptr_xfcerc, "Show", ptr_chipfeature->show);

                       str_tmp = g_strdup_printf("%.2f", ptr_chipfeature->min_value);
                       xfce_rc_write_entry (ptr_xfcerc, "Min", str_tmp);
                       g_free (str_tmp);

                       str_tmp = g_strdup_printf("%.2f", ptr_chipfeature->max_value);
                       xfce_rc_write_entry (ptr_xfcerc, "Max", str_tmp);
                       g_free (str_tmp);
                    } /* end if */

                } /* end for idx_features */

            } /* end for idx_chips */

            xfce_rc_close (ptr_xfcerc);

            TRACE ("leaves sensors_write_config");
        }
    }
}


/* -------------------------------------------------------------------------- */
void
sensors_read_general_config (XfceRc *ptr_xfceresources, t_sensors *ptr_sensors)
{
    const gchar *str_value;

    TRACE ("enters sensors_read_general_config");

    g_return_if_fail(ptr_xfceresources!=NULL);

    g_return_if_fail(ptr_sensors!=NULL);

    if (xfce_rc_has_group (ptr_xfceresources, "General") ) {

        xfce_rc_set_group (ptr_xfceresources, "General");

        ptr_sensors->show_title = xfce_rc_read_bool_entry (ptr_xfceresources, "Show_Title", TRUE);

        ptr_sensors->show_labels = xfce_rc_read_bool_entry (ptr_xfceresources, "Show_Labels", TRUE);

        ptr_sensors->display_values_type = xfce_rc_read_int_entry (ptr_xfceresources, "Use_Bar_UI", 0);

        ptr_sensors->show_colored_bars = xfce_rc_read_bool_entry (ptr_xfceresources, "Show_Colored_Bars", FALSE);

        ptr_sensors->scale = xfce_rc_read_int_entry (ptr_xfceresources, "Scale", 0);

        if ((str_value = xfce_rc_read_entry (ptr_xfceresources, "str_fontsize", NULL)) && *str_value) {
            g_free(ptr_sensors->str_fontsize);
            ptr_sensors->str_fontsize = g_strdup(str_value);
        }

        if ((str_value = xfce_rc_read_entry (ptr_xfceresources, "Font", NULL)) && *str_value) {
            if (font)
                g_free(font); // font is initialized to NULL
            font = g_strdup(str_value); // in tacho.h for the tachometers
        }
        else if (font==NULL)
            font = g_strdup("Sans 11");

        ptr_sensors->val_fontsize = xfce_rc_read_int_entry (ptr_xfceresources,
                                                 "val_fontsize", 2);

        ptr_sensors->lines_size = xfce_rc_read_int_entry (ptr_xfceresources, "Lines_Size", 3);

        ptr_sensors->cover_panel_rows = xfce_rc_read_bool_entry (ptr_xfceresources, "Cover_All_Panel_Rows", FALSE);

        ptr_sensors->sensors_refresh_time = xfce_rc_read_int_entry (ptr_xfceresources, "Update_Interval",
                                                  60);

        ptr_sensors->exec_command = xfce_rc_read_bool_entry (ptr_xfceresources, "Exec_Command", TRUE);

        ptr_sensors->show_units= xfce_rc_read_bool_entry (ptr_xfceresources, "Show_Units", TRUE);

        ptr_sensors->show_smallspacings= xfce_rc_read_bool_entry (ptr_xfceresources, "Small_Spacings", FALSE);

        if ((str_value = xfce_rc_read_entry (ptr_xfceresources, "Command_Name", NULL)) && *str_value) {
            g_free(ptr_sensors->command_name);
            ptr_sensors->command_name = g_strdup (str_value);
        }

        if (!ptr_sensors->suppressmessage)
            ptr_sensors->suppressmessage = xfce_rc_read_bool_entry (ptr_xfceresources, "Suppress_Hddtemp_Message", FALSE);

        //if (!ptr_sensors->suppresstooltip)
            ptr_sensors->suppresstooltip = xfce_rc_read_bool_entry (ptr_xfceresources, "Suppress_Tooltip", FALSE);

        ptr_sensors->preferred_width = xfce_rc_read_int_entry (ptr_xfceresources, "Preferred_Width", 400);
        ptr_sensors->preferred_height = xfce_rc_read_int_entry (ptr_xfceresources, "Preferred_Height", 400);

        //num_chips = xfce_rc_read_int_entry (ptr_xfceresources, "Number_Chips", 0);
        /* or could use 1 or the always existent dummy entry */
    }

    TRACE ("leaves sensors_read_general_config");
}


/* -------------------------------------------------------------------------- */
void
sensors_read_preliminary_config (XfcePanelPlugin *ptr_panelplugin, t_sensors *ptr_sensors)
{
    XfceRc *ptr_xfceresource;

    TRACE ("enters sensors_read_preliminary_config");

    if (ptr_panelplugin)
    {
        g_return_if_fail(ptr_sensors!=NULL);

        if ((ptr_sensors->plugin_config_file))
        {
            ptr_xfceresource = xfce_rc_simple_open (ptr_sensors->plugin_config_file, TRUE);

            if (ptr_xfceresource && xfce_rc_has_group (ptr_xfceresource, "General") ) {
                xfce_rc_set_group (ptr_xfceresource, "General");
                ptr_sensors->suppressmessage = xfce_rc_read_bool_entry (ptr_xfceresource, "Suppress_Hddtemp_Message", FALSE);
            }
        }
    }

    TRACE ("leaves sensors_read_preliminary_config");
}


/* -------------------------------------------------------------------------- */
// TODO: modify to store chipname as indicator and access features by acpitz-1_Feature0 etc.
// this will require differently storing the stuff as well.
// targeted for 1.3 or 1.4 release
void
sensors_read_config (XfcePanelPlugin *ptr_panelplugin, t_sensors *ptr_sensors)
{
    const gchar *str_value;
    XfceRc *ptr_xfceresource;
    gint idx_chip, idx_feature, idx_chiptmp;
    gchar str_rcchip[8], str_feature[20];
    gchar* str_sensorname=NULL;
    gint num_sensorchip, id, address;
    t_chip *ptr_chip;
    t_chipfeature *ptr_chipfeature;

    TRACE ("enters sensors_read_config");

    g_return_if_fail(ptr_panelplugin!=NULL);

    g_return_if_fail(ptr_sensors!=NULL);

    if (!(ptr_sensors->plugin_config_file))
        return;

    ptr_xfceresource = xfce_rc_simple_open (ptr_sensors->plugin_config_file, TRUE);

    if (!ptr_xfceresource)
        return;

    sensors_read_general_config (ptr_xfceresource, ptr_sensors);

    for (idx_chip = 0; idx_chip<ptr_sensors->num_sensorchips; idx_chip++) {

        g_snprintf (str_rcchip, 8, "Chip%d", idx_chip);

        if (xfce_rc_has_group (ptr_xfceresource, str_rcchip)) {

            xfce_rc_set_group (ptr_xfceresource, str_rcchip);

            num_sensorchip=0;

            if ((str_value = xfce_rc_read_entry (ptr_xfceresource, "Name", NULL)) && *str_value) {
                str_sensorname = g_strdup (str_value);
            }

            num_sensorchip = (gint) xfce_rc_read_int_entry (ptr_xfceresource, "Number", 0);

            /* assert that str_file does not contain more information
              than does exist on system */
              /* ??? At least, it works. */
            g_return_if_fail (num_sensorchip < ptr_sensors->num_sensorchips);

            /* now featuring enhanced string comparison */
            idx_chiptmp = 0;
            do {
              ptr_chip = (t_chip *) g_ptr_array_index (ptr_sensors->chips, idx_chiptmp++);
              if (ptr_chip==NULL || idx_chiptmp==ptr_sensors->num_sensorchips)
                  break;
            }
            while (ptr_chip!=NULL && str_sensorname != NULL && strcmp(ptr_chip->sensorId, str_sensorname) != 0 );

            if ( ptr_chip!=NULL && str_sensorname != NULL && strcmp(ptr_chip->sensorId, str_sensorname)==0 ) {

                for (idx_feature=0; idx_feature<ptr_chip->num_features; idx_feature++) {
                    ptr_chipfeature = (t_chipfeature *) g_ptr_array_index (
                                                        ptr_chip->chip_features,
                                                        idx_feature);
                    g_assert (ptr_chipfeature!=NULL);

                    g_snprintf (str_feature, 20, "%s_Feature%d", str_rcchip, idx_feature);

                    if (xfce_rc_has_group (ptr_xfceresource, str_feature)) {
                        xfce_rc_set_group (ptr_xfceresource, str_feature);

                        address=0;

                        if ((str_value = xfce_rc_read_entry (ptr_xfceresource, "DeviceName", NULL))
                            && *str_value) {
                            if (ptr_chipfeature->devicename)
                                g_free (ptr_chipfeature->devicename);
                            ptr_chipfeature->devicename = g_strdup(str_value);
                        }

                        if ((str_value = xfce_rc_read_entry (ptr_xfceresource, "Name", NULL))
                                && *str_value) {
                            if (ptr_chipfeature->name)
                                g_free (ptr_chipfeature->name);
                            ptr_chipfeature->name = g_strdup (str_value);
                        }

                        if ((str_value = xfce_rc_read_entry (ptr_xfceresource, "Color", NULL))
                                && *str_value) {
                            if (ptr_chipfeature->color)
                                g_free (ptr_chipfeature->color);
                            ptr_chipfeature->color = g_strdup (str_value);
                        }

                        ptr_chipfeature->show =
                            xfce_rc_read_bool_entry (ptr_xfceresource, "Show", FALSE);

                        if ((str_value = xfce_rc_read_entry (ptr_xfceresource, "Min", NULL))
                                && *str_value)
                            ptr_chipfeature->min_value = atof (str_value);

                        if ((str_value = xfce_rc_read_entry (ptr_xfceresource, "Max", NULL))
                                && *str_value)
                            ptr_chipfeature->max_value = atof (str_value);


                    } /* end if rc_grup has str_feature*/

                } /* end for str_features */

            } /* end if chip && strcmp */

            if (str_sensorname != NULL)
              g_free (str_sensorname);

        } /* end if xfce_rc_has_group (ptr_xfceresource, str_rcchip) */

    } /* end for num_sensorchips */

    xfce_rc_close (ptr_xfceresource);

    if (!ptr_sensors->exec_command) {
        g_signal_handler_block ( G_OBJECT(ptr_sensors->eventbox), ptr_sensors->doubleclick_id );
    }

    TRACE ("leaves sensors_read_config");
}
