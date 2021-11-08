/* callbacks.h
 * Part of xfce4-sensors-plugin
 *
 * Copyright (c) 2008-2017 Fabian Nowak <timystery@arcor.de>
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

#include <glib.h>
#include <gtk/gtk.h>

/* Package includes */
#include <sensors-interface-common.h>

/* TODO: there should also be some "private" callbacks such as closing/qutting the application. */

/**
 * Callback when new font is set in application and must be updated for the tacho widgets.
 */
void on_font_set (GtkFontButton *widget, const Ptr<t_sensors_dialog> &dialog);

/* Functions for implementing the sensors interface common callback functions */

/**
 * Implementation of interface callback adjustment_value_changed
 */
void adjustment_value_changed_  (GtkAdjustment *adjustment, const Ptr<t_sensors_dialog> &dialog); // for update timer box

/**
 * Implementation of interface callback sensor_entry_changed_
 */
void sensor_entry_changed_ (GtkWidget *widget, const Ptr<t_sensors_dialog> &dialog);

/**
 * Implementation of interface callback list_cell_text_edited_
 */
void list_cell_text_edited_ (GtkCellRendererText *cellrenderertext, gchar *path,
                             gchar *new_text, const Ptr<t_sensors_dialog> &dialog);

/**
 * Implementation of interface callback list_cell_toggle_
 */
void list_cell_toggle_ (GtkCellRendererToggle *cell, gchar *path, const Ptr<t_sensors_dialog> &dialog);

/**
 * Implementation of interface callback list_cell_color_edited_
 */
void list_cell_color_edited_ (GtkCellRendererText *cellrenderertext, const gchar *path,
                              const gchar *new_color, const Ptr<t_sensors_dialog> &dialog);

/**
 * Implementation of interface callback minimum_changed_
 */
void minimum_changed_ (GtkCellRendererText *cellrenderertext, gchar *path,
                       gchar *new_value, const Ptr<t_sensors_dialog> &dialog);

/**
 * Implementation of interface callback maximum_changed
 */
void maximum_changed_ (GtkCellRendererText *cellrenderertext, gchar *path,
                       gchar *new_value, const Ptr<t_sensors_dialog> &dialog);

/**
 * Implementation of interface callback temperature_unit_change
 */
void temperature_unit_change_ (GtkToggleButton *widget, const Ptr<t_sensors_dialog> &dialog);

#endif /* __CALLBACKS_H */
