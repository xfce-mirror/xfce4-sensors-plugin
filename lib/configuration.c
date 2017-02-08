/* $Id$ */
/* File configuration.c
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


gint
get_Id_from_address (gint chipnumber, gint addr_chipfeature, t_sensors *ptr_sensors)
{
    gint idx_feature = -1;
    t_chip *ptr_chip = NULL;
    t_chipfeature *ptr_chipfeature = NULL;

    TRACE ("enters get_Id_from_address");

    ptr_chip = (t_chip *) g_ptr_array_index (ptr_sensors->chips, chipnumber);

    if (ptr_chip)
    {
        for (idx_feature=0; idx_feature<ptr_chip->num_features; idx_feature++) {
            ptr_chipfeature = g_ptr_array_index(ptr_chip->chip_features, idx_feature);
            if (ptr_chipfeature)
            {
                if (addr_chipfeature == ptr_chipfeature->address) {
                    break;
                }
            }
        }
    }

    TRACE ("leaves get_Id_from_address with %d", idx_feature);
    return idx_feature;
}


void
sensors_write_config (t_sensors *ptr_sensors)
{
    XfceRc *ptr_xfcerc;
    gchar *str_file, *str_tmp, rc_chip[8], feature[20];
    gint i, j;
    t_chip *chip;
    t_chipfeature *chipfeature;

    TRACE ("enters sensors_write_config");

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

            for (i=0; i<ptr_sensors->num_sensorchips; i++) {

                chip = (t_chip *) g_ptr_array_index(ptr_sensors->chips, i);
                g_assert (chip!=NULL);

                g_snprintf (rc_chip, 8, "Chip%d", i);

                xfce_rc_set_group (ptr_xfcerc, rc_chip);

                xfce_rc_write_entry (ptr_xfcerc, "Name", chip->sensorId);

                /* number of sensors is still limited */
                xfce_rc_write_int_entry (ptr_xfcerc, "Number", i);

                for (j=0; j<chip->num_features; j++) {
                    chipfeature = g_ptr_array_index(chip->chip_features, j);
                    g_assert (chipfeature!=NULL);

                    if (chipfeature->show) {

                       g_snprintf (feature, 20, "%s_Feature%d", rc_chip, j);

                       xfce_rc_set_group (ptr_xfcerc, feature);

                       xfce_rc_write_int_entry (ptr_xfcerc, "Id", get_Id_from_address(i, j, ptr_sensors));

                       /* only use this if no hddtemp sensor */
                       /* or do only use this , if it is an lmsensors device. whatever. */
                       if ( strcmp(chip->sensorId, _("Hard disks")) != 0 ) /* chip->name? */
                            xfce_rc_write_int_entry (ptr_xfcerc, "Address", j);
                        else
                            xfce_rc_write_entry (ptr_xfcerc, "DeviceName", chipfeature->devicename);

                       xfce_rc_write_entry (ptr_xfcerc, "Name", chipfeature->name);

                       xfce_rc_write_entry (ptr_xfcerc, "Color", chipfeature->color);

                       xfce_rc_write_bool_entry (ptr_xfcerc, "Show", chipfeature->show);

                       str_tmp = g_strdup_printf("%.2f", chipfeature->min_value);
                       xfce_rc_write_entry (ptr_xfcerc, "Min", str_tmp);
                       g_free (str_tmp);

                       str_tmp = g_strdup_printf("%.2f", chipfeature->max_value);
                       xfce_rc_write_entry (ptr_xfcerc, "Max", str_tmp);
                       g_free (str_tmp);
                    } /* end if */

                } /* end for j */

            } /* end for i */

            xfce_rc_close (ptr_xfcerc);

            TRACE ("leaves sensors_write_config");
        }
    }
}


void
sensors_read_general_config (XfceRc *rc, t_sensors *sensors)
{
    const gchar *value;

    TRACE ("enters sensors_read_general_config");

    if (xfce_rc_has_group (rc, "General") ) {

        xfce_rc_set_group (rc, "General");

        sensors->show_title = xfce_rc_read_bool_entry (rc, "Show_Title", TRUE);

        sensors->show_labels = xfce_rc_read_bool_entry (rc, "Show_Labels", TRUE);

        sensors->display_values_type = xfce_rc_read_int_entry (rc, "Use_Bar_UI", 0);

        sensors->show_colored_bars = xfce_rc_read_bool_entry (rc, "Show_Colored_Bars", FALSE);

        sensors->scale = xfce_rc_read_int_entry (rc, "Scale", 0);

        if ((value = xfce_rc_read_entry (rc, "str_fontsize", NULL)) && *value) {
            g_free(sensors->str_fontsize);
            sensors->str_fontsize = g_strdup(value);
        }

        if ((value = xfce_rc_read_entry (rc, "Font", NULL)) && *value) {
            //g_free(sensors->font); // font is initialized to NULL
            font = g_strdup(value); // in tacho.h for the tachometers
        }

        sensors->val_fontsize = xfce_rc_read_int_entry (rc,
                                                 "val_fontsize", 2);

        sensors->lines_size = xfce_rc_read_int_entry (rc, "Lines_Size", 3);

        sensors->sensors_refresh_time = xfce_rc_read_int_entry (rc, "Update_Interval",
                                                  60);

        sensors->exec_command = xfce_rc_read_bool_entry (rc, "Exec_Command", TRUE);

        sensors->show_units= xfce_rc_read_bool_entry (rc, "Show_Units", TRUE);

        sensors->show_smallspacings= xfce_rc_read_bool_entry (rc, "Small_Spacings", FALSE);

        if ((value = xfce_rc_read_entry (rc, "Command_Name", NULL)) && *value) {
            g_free(sensors->command_name);
            sensors->command_name = g_strdup (value);
        }

        if (!sensors->suppressmessage)
            sensors->suppressmessage = xfce_rc_read_bool_entry (rc, "Suppress_Hddtemp_Message", FALSE);

        if (!sensors->suppresstooltip)
            sensors->suppresstooltip = xfce_rc_read_bool_entry (rc, "Suppress_Tooltip", FALSE);

        sensors->preferred_width = xfce_rc_read_int_entry (rc, "Preferred_Width", 400);
        sensors->preferred_height = xfce_rc_read_int_entry (rc, "Preferred_Height", 400);

        //num_chips = xfce_rc_read_int_entry (rc, "Number_Chips", 0);
        /* or could use 1 or the always existent dummy entry */
    }

    TRACE ("leaves sensors_read_general_config");
}


void
sensors_read_preliminary_config (XfcePanelPlugin *plugin, t_sensors *sensors)
{
    gchar *file;
    XfceRc *rc;

    TRACE ("enters sensors_read_preliminary_config");

    if (plugin==NULL)
        return;

    if (!(file = sensors->plugin_config_file))
        return;

    rc = xfce_rc_simple_open (file, TRUE);
    g_free (file);

    if (!rc)
        return;

    if (xfce_rc_has_group (rc, "General") ) {
        xfce_rc_set_group (rc, "General");
        sensors->suppressmessage = xfce_rc_read_bool_entry (rc, "Suppress_Hddtemp_Message", FALSE);
    }

    TRACE ("leaves sensors_read_preliminary_config");
}


// TODO: modify to store chipname as indicator and access features by acpitz-1_Feature0 etc.
// this will require differently storing the stuff as well.
// targeted for 1.1 or 1.2 release
void
sensors_read_config (XfcePanelPlugin *plugin, t_sensors *sensors)
{
    const gchar *value;
    gchar *file;
    XfceRc *rc;
    gint i, j, k;
    gchar rc_chip[8], feature[20];
    gchar* sensorName=NULL;
    gint num_sensorchip, id, address;
    t_chip *chip;
    t_chipfeature *chipfeature;

    TRACE ("enters sensors_read_config");

    if (!(file = sensors->plugin_config_file))
        return;

    rc = xfce_rc_simple_open (file, TRUE);
    g_free (file);

    if (!rc)
        return;

    sensors_read_general_config (rc, sensors);

    for (i = 0; i<sensors->num_sensorchips; i++) {

        g_snprintf (rc_chip, 8, "Chip%d", i);

        if (xfce_rc_has_group (rc, rc_chip)) {

            xfce_rc_set_group (rc, rc_chip);

            num_sensorchip=0;

            if ((value = xfce_rc_read_entry (rc, "Name", NULL)) && *value) {
                sensorName = g_strdup (value);
            }

            num_sensorchip = (gint) xfce_rc_read_int_entry (rc, "Number", 0);

            /* assert that file does not contain more information
              than does exist on system */
              /* ??? At least, it works. */
              //DBG("number of chip from file: %d, number of expected or known chips: %d.\n", num_sensorchip, sensors->num_sensorchips);
            g_return_if_fail (num_sensorchip < sensors->num_sensorchips);
            //DBG ("Success.\n");

            /* now featuring enhanced string comparison */
            //g_assert (chip!=NULL);
            k = 0;
            do {
              chip = (t_chip *) g_ptr_array_index (sensors->chips, k++);
              if (chip==NULL || k==sensors->num_sensorchips)
                  break;
              }
            while (chip!=NULL && sensorName != NULL && strcmp(chip->sensorId, sensorName) != 0 );

            if ( chip!=NULL && sensorName != NULL && strcmp(chip->sensorId, sensorName)==0 ) {

                for (j=0; j<chip->num_features; j++) {
                    chipfeature = (t_chipfeature *) g_ptr_array_index (
                                                        chip->chip_features,
                                                        j);
                    g_assert (chipfeature!=NULL);

                    g_snprintf (feature, 20, "%s_Feature%d", rc_chip, j);

                    if (xfce_rc_has_group (rc, feature)) {
                        xfce_rc_set_group (rc, feature);

                        address=0;

                        id = (gint) xfce_rc_read_int_entry (rc, "Id", 0);

                        if ( strcmp(chip->sensorId, _("Hard disks")) != 0 )
                            address = (gint) xfce_rc_read_int_entry (rc, "Address", 0);
                        else

                         /* FIXME: compare strings, or also have hddtemp and acpi store numeric values */

                        /* assert correctly saved file */
                        if (strcmp(chip->sensorId, _("Hard disks")) != 0) { /* chip->name? */
                            chipfeature = g_ptr_array_index(chip->chip_features, id);
                            /* FIXME: it might be necessary to use sensors->addresses here */
                            /* g_return_if_fail
                                (chipfeature->address == address); */
                            if (chipfeature->address != address)
                                continue;
                        }
                        else if ((value = xfce_rc_read_entry (rc, "DeviceName", NULL))
                            && *value) {
                            if (chipfeature->devicename)
                                free (chipfeature->devicename);
                            chipfeature->devicename = g_strdup(value);
                            /* g_free (value); */
                        }

                        if ((value = xfce_rc_read_entry (rc, "Name", NULL))
                                && *value) {
                            if (chipfeature->name)
                                free (chipfeature->name);
                            chipfeature->name = g_strdup (value);
                            /* g_free (value); */
                        }

                        if ((value = xfce_rc_read_entry (rc, "Color", NULL))
                                && *value) {
                            if (chipfeature->color)
                                free (chipfeature->color);
                            chipfeature->color = g_strdup (value);
                            /* g_free (value); */
                        }

                        chipfeature->show =
                            xfce_rc_read_bool_entry (rc, "Show", FALSE);

                        if ((value = xfce_rc_read_entry (rc, "Min", NULL))
                                && *value)
                            chipfeature->min_value = atof (value);

                        if ((value = xfce_rc_read_entry (rc, "Max", NULL))
                                && *value)
                            chipfeature->max_value = atof (value);


                    } /* end if rc_grup has feature*/

                } /* end for features */

            } /* end if chip && strcmp */

            if (sensorName != NULL)
              g_free (sensorName);

        } /* end if xfce_rc_has_group (rc, rc_chip) */

    } /* end for num_sensorchips */

    xfce_rc_close (rc);

    if (!sensors->exec_command) {
        g_signal_handler_block ( G_OBJECT(sensors->eventbox), sensors->doubleclick_id );
    }

    TRACE ("leaves sensors_read_config");
}
