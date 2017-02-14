/* File: interface.h
 *
 *  Copyright 2008-2017 Fabian Nowak (timystery@arcor.de)
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

/* Gtk/Glib includes */
#include <gtk/gtk.h>

/* Package includes */
#include <sensors-interface-common.h>

GtkWidget * create_main_window (t_sensors_dialog *sd);

gboolean draw_callback(GtkWidget    *widget,
               cairo_t *cr,
               gpointer      user_data);
