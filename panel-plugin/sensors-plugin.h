/* File: sensors-plugin.h
 *
 * Copyright 2004-2017 Fabian Nowak (timystery@arcor.de)
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

#ifndef XFCE4_SENSORS_SENSORS_H
#define XFCE4_SENSORS_SENSORS_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif



#define APP_NAME N_("Sensors Plugin")


/* Functions for implementing the sensors interface common callback functions */

void adjustment_value_changed_ (GtkWidget *ptr_widget, t_sensors_dialog *v);

void sensor_entry_changed_ (GtkWidget *ptr_widget, t_sensors_dialog *ptr_sensorsdialog);

void list_cell_text_edited_ (GtkCellRendererText *cellrenderertext,
                             gchar *ptr_str_newtext, gchar *new_text, t_sensors_dialog *ptr_sensorsdialog);

void list_cell_toggle_ (GtkCellRendererToggle *ptr_cellrenderertoggle, gchar *ptr_str_path,
                        t_sensors_dialog *ptr_sensorsdialog);

void list_cell_color_edited_ (GtkCellRendererText *cellrenderertext, gchar *ptr_str_path,
                              gchar *ptr_str_newcolor, t_sensors_dialog *v);

void minimum_changed_ (GtkCellRendererText *cellrenderertext, gchar *ptr_str_path,
                       gchar *ptr_str_newmin, t_sensors_dialog *ptr_sensorsdialog);

void maximum_changed_ (GtkCellRendererText *cellrenderertext, gchar *ptr_str_path,
                       gchar *ptr_str_newmax, t_sensors_dialog *ptr_sensorsdialog);

void temperature_unit_change_ (GtkWidget *ptr_widget, t_sensors_dialog *ptr_sensorsdialog);


#endif /* XFCE4_SENSORS_SENSORS_H */
