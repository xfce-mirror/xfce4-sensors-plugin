/* File: callbacks.h
 *
 * Copyright 2008-2017 Fabian Nowak (timystery@arcor.de)
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

#ifndef __CALLBACKS_H
#define __CALLBACKS_H

#include <gtk/gtk.h>

/* Package includes */
#include <sensors-interface-common.h>


/* there should also be some "private" callbacks such as closing/qutting
 * the application.
 */
/**
 * Callback when main window is closed; shall quit the Gtk main routine and
 * perhaps also free some further allocated structures.
 * @param dlg: pointer to dialog widget to be destroyed
 * @param response: event number of close event
 * @param dialog: pointer to helpful sensors dialog structure
 */
void on_main_window_response (GtkWidget *dlg, int response, t_sensors_dialog *dialog);

/**
 * Callback when new font is set in application and must be updated for the
 * tacho widgets.
 * @param widget: pointer to font button widget
 * @param data: pointer to sensors dialog structure
 */
void on_font_set (GtkWidget *widget, gpointer data);

/* Functions for implementing the sensors interface common callback functions */

/**
 * Implementation of interface callback adjustment_value_changed
 */
void adjustment_value_changed_  (GtkWidget *widget, t_sensors_dialog *dialog); // for update timer box

/**
 * Implementation of interface callback sensor_entry_changed_
 */
void sensor_entry_changed_ (GtkWidget *widget, t_sensors_dialog *dialog);

/**
 * Implementation of interface callback list_cell_text_edited_
 */
void list_cell_text_edited_ (GtkCellRendererText *cellrenderertext, gchar *path,
                             gchar *new_text, t_sensors_dialog *dialog);

/**
 * Implementation of interface callback list_cell_toggle_
 */
void list_cell_toggle_ (GtkCellRendererToggle *cell, gchar *path, t_sensors_dialog *dialog);

/**
 * Implementation of interface callback list_cell_color_edited_
 */
void list_cell_color_edited_ (GtkCellRendererText *cellrenderertext, gchar *path,
                              gchar *new_color, t_sensors_dialog *dialog);

/**
 * Implementation of interface callback minimum_changed_
 */
void minimum_changed_ (GtkCellRendererText *cellrenderertext, gchar *path,
                       gchar *new_value, t_sensors_dialog *dialog);

/**
 * Implementation of interface callback maximum_changed
 */
void maximum_changed_ (GtkCellRendererText *cellrenderertext, gchar *path,
                       gchar *new_value, t_sensors_dialog *dialog);

/**
 * Implementation of interface callback temperature_unit_change
 */
void temperature_unit_change_ (GtkWidget *widget, t_sensors_dialog *dialog);


#endif /* __CALLBACKS_H */
