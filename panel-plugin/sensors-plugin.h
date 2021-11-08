/* sensors-plugin.h
 * Part of xfce4-sensors-plugin
 *
 * Copyright (c) 2004-2017 Fabian Nowak <timystery@arcor.de>
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

#include <libxfce4util/libxfce4util.h>

#define APP_NAME N_("Sensors Plugin")

/* Functions for implementing the sensors interface common callback functions */

void adjustment_value_changed_ (GtkAdjustment *adjustment, const Ptr<t_sensors_dialog> &dialog);

void sensor_entry_changed_ (GtkWidget *widget, const Ptr<t_sensors_dialog> &dialog);

void list_cell_text_edited_ (GtkCellRendererText *cell_renderer_text, gchar *path,
                             gchar *new_text, const Ptr<t_sensors_dialog> &dialog);

void list_cell_toggle_ (GtkCellRendererToggle *cell_renderer_toggle, gchar *path,
                        const Ptr<t_sensors_dialog> &dialog);

void list_cell_color_edited_ (GtkCellRendererText *cell_renderer_text, const gchar *path,
                              const gchar *newcolor, const Ptr<t_sensors_dialog> &dialog);

void minimum_changed_ (GtkCellRendererText *cell_renderer_text, gchar *path,
                       gchar *newmin, const Ptr<t_sensors_dialog> &dialog);

void maximum_changed_ (GtkCellRendererText *cellrenderertext, gchar *path,
                       gchar *newmax, const Ptr<t_sensors_dialog> &dialog);

void temperature_unit_change_ (GtkToggleButton *widget, const Ptr<t_sensors_dialog> &dialog);

#endif /* XFCE4_SENSORS_SENSORS_H */
