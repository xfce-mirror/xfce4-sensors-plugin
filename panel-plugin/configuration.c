/* File configuration.c
 *
 *  Copyright 2004-2007 Fabian Nowak (timystery@arcor.de)
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


#include "configuration.h"

#include <stdlib.h>


gint
get_Id_from_address (gint chipnumber, gint addr, t_sensors *sensors)
{
    gint feature;
    t_chip *chip;
    t_chipfeature *chipfeature;

    TRACE ("enters get_Id_from_address");

    chip = (t_chip *) g_ptr_array_index (sensors->chips, chipnumber);

    for (feature=0; feature<chip->num_features; feature++) {
        chipfeature = g_ptr_array_index(chip->chip_features, feature);
        if (addr == chipfeature->address) {
            TRACE ("leaves get_Id_from_address with %d", feature);
            return feature;
        }
    }

    TRACE ("leaves get_Id_from_address with -1");

    return (gint) -1;
}


/* Write the configuration at exit */
void
sensors_write_config (XfcePanelPlugin *plugin, t_sensors *sensors)
{
    XfceRc *rc;
    char *file, *tmp, rc_chip[8], feature[20];
    int i, j;
    t_chip *chip;
    t_chipfeature *chipfeature;

    TRACE ("enters sensors_write_config");

    if (!(file = xfce_panel_plugin_save_location (plugin, TRUE))) {
        TRACE ("leaves sensors_write_config: No file location specified.");
        return;
    }

    unlink (file);

    rc = xfce_rc_simple_open (file, FALSE);
    g_free (file);

    if (!rc) {
        TRACE ("leaves sensors_write_config: No rc file opened");
        return;
    }

    xfce_rc_set_group (rc, "General");

    xfce_rc_write_bool_entry (rc, "Show_Title", sensors->show_title);

    xfce_rc_write_bool_entry (rc, "Show_Labels", sensors->show_labels);

    xfce_rc_write_bool_entry (rc, "Use_Bar_UI", sensors->display_values_graphically);

    xfce_rc_write_bool_entry (rc, "Show_Colored_Bars", sensors->show_colored_bars);

    xfce_rc_write_int_entry (rc, "Scale", sensors->scale);

    xfce_rc_write_entry (rc, "Font_Size", sensors->font_size);

    xfce_rc_write_int_entry (rc, "Font_Size_Numerical",
                                sensors->font_size_numerical);

    xfce_rc_write_int_entry (rc, "Update_Interval", sensors->sensors_refresh_time);

    xfce_rc_write_bool_entry (rc, "Exec_Command", sensors->exec_command);

    xfce_rc_write_entry (rc, "Command_Name", sensors->command_name);

    xfce_rc_write_int_entry (rc, "Number_Chips", sensors->num_sensorchips);


    for (i=0; i<sensors->num_sensorchips; i++) {

        chip = (t_chip *) g_ptr_array_index(sensors->chips, i);
        g_assert (chip!=NULL);

        g_snprintf (rc_chip, 8, "Chip%d", i);

        xfce_rc_set_group (rc, rc_chip);

        xfce_rc_write_entry (rc, "Name", chip->sensorId);

        /* number of sensors is still limited */
        xfce_rc_write_int_entry (rc, "Number", i);

        for (j=0; j<chip->num_features; j++) {
            chipfeature = g_ptr_array_index(chip->chip_features, j);
            g_assert (chipfeature!=NULL);

            if (chipfeature->show == TRUE) {

               g_snprintf (feature, 20, "%s_Feature%d", rc_chip, j);

               xfce_rc_set_group (rc, feature);

               xfce_rc_write_int_entry (rc, "Id", get_Id_from_address(i, j, sensors));

               /* only use this if no hddtemp sensor */
               if ( strcmp(chipfeature->name, _("Hard disks")) != 0 )
                    xfce_rc_write_int_entry (rc, "Address", j);

               xfce_rc_write_entry (rc, "Name", chipfeature->name);

               xfce_rc_write_entry (rc, "Color", chipfeature->color);

               xfce_rc_write_bool_entry (rc, "Show", chipfeature->show);

                tmp = g_strdup_printf("%.2f", chipfeature->min_value);
               xfce_rc_write_entry (rc, "Min", tmp);

               tmp = g_strdup_printf("%.2f", chipfeature->max_value);
               xfce_rc_write_entry (rc, "Max", tmp);
            } /* end if */

        } /* end for j */

    } /* end for i */

    xfce_rc_close (rc);

    TRACE ("leaves sensors_write_config");
}


void
sensors_read_general_config (XfceRc *rc, t_sensors *sensors)
{
    const char *value;
    gint num_chips;

    TRACE ("enters sensors_read_general_config");

    if (xfce_rc_has_group (rc, "General") ) {

        xfce_rc_set_group (rc, "General");

        sensors->show_title = xfce_rc_read_bool_entry (rc, "Show_Title", TRUE);

        sensors->show_labels = xfce_rc_read_bool_entry (rc, "Show_Labels", TRUE);

        sensors->display_values_graphically = xfce_rc_read_bool_entry (rc, "Use_Bar_UI", FALSE);

        sensors->show_colored_bars = xfce_rc_read_bool_entry (rc, "Show_Colored_Bars", FALSE);

        sensors->scale = xfce_rc_read_int_entry (rc, "Scale", 0);

        if ((value = xfce_rc_read_entry (rc, "Font_Size", NULL)) && *value) {
            sensors->font_size = g_strdup(value);
        }

        sensors->font_size_numerical = xfce_rc_read_int_entry (rc,
                                                 "Font_Size_Numerical", 2);

        sensors->sensors_refresh_time = xfce_rc_read_int_entry (rc, "Update_Interval",
                                                  60);

        sensors->exec_command = xfce_rc_read_bool_entry (rc, "Exec_Command", TRUE);

        if ((value = xfce_rc_read_entry (rc, "Command_Name", NULL)) && *value) {
            sensors->command_name = g_strdup (value);
        }

        num_chips = xfce_rc_read_int_entry (rc, "Number_Chips", 0);
        /* or could use 1 or the always existent dummy entry */
    }

    TRACE ("leaves sensors_read_general_config");
}


/* Read the configuration file at init */
void
sensors_read_config (XfcePanelPlugin *plugin, t_sensors *sensors)
{
    const char *value;
    char *file;
    XfceRc *rc;
    int i, j;
    char rc_chip[8], feature[20];
    gchar* sensorName=NULL;
    gint num_sensorchip, id, address;
    t_chip *chip;
    t_chipfeature *chipfeature;

    TRACE ("enters sensors_read_config");

    if (!(file = xfce_panel_plugin_lookup_rc_file (plugin)))
        return;

    rc = xfce_rc_simple_open (file, TRUE);
    g_free (file);

    if (!rc)
        return;

    sensors_read_general_config (rc, sensors);

    for (i = 0; i<sensors->num_sensorchips; i++) {
        chip = (t_chip *) g_ptr_array_index (sensors->chips, i);
        if (chip==NULL)
            break;

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
            g_return_if_fail (num_sensorchip < sensors->num_sensorchips);

            /* now featuring enhanced string comparison */
            g_assert (chip!=NULL);
            if ( strcmp(chip->sensorId, sensorName)==0 ) {

                for (j=0; j<chip->num_features; j++) {
                    chipfeature = (t_chipfeature *) g_ptr_array_index (
                                                        chip->chip_features,
                                                        j);
                    g_assert (chipfeature!=NULL);

                    g_snprintf (feature, 20, "%s_Feature%d", rc_chip, j);

                    if (xfce_rc_has_group (rc, feature)) {
                        xfce_rc_set_group (rc, feature);

                        id=0; address=0;

                        id = (gint) xfce_rc_read_int_entry (rc, "Id", 0);

                        if ( strcmp(chip->name, _("Hard disks")) != 0 )
                            address = (gint) xfce_rc_read_int_entry (rc, "Address", 0);
                        else
                         /* FIXME: compare strings, or also have hddtmep and acpi store numeric values */

                        /* assert correctly saved file */
                        if (strcmp(chip->name, _("Hard disks")) != 0) {
                            chipfeature = g_ptr_array_index(chip->chip_features, id);
                            /* FIXME: it might be necessary to use sensors->addresses here */
                            /* g_return_if_fail
                                (chipfeature->address == address); */
                            if (chipfeature->address != address)
                                continue;
                        }

                        if ((value = xfce_rc_read_entry (rc, "Name", NULL))
                                && *value) {
                            chipfeature->name = g_strdup (value);
                            /* g_free (value); */
                        }

                        if ((value = xfce_rc_read_entry (rc, "Color", NULL))
                                && *value) {
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

                    } /* end if */

                } /* end for */

            } /* end if */

            g_free (sensorName);

        } /* end if */

    } /* end for */

    xfce_rc_close (rc);

    if (!sensors->exec_command) {
        g_signal_handler_block ( G_OBJECT(sensors->eventbox), sensors->doubleclick_id );
    }

    TRACE ("leaves sensors_read_config");
}