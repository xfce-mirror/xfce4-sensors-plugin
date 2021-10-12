/* interface.h
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

#include <glib.h>
#include <gtk/gtk.h>

/* Package includes */
#include <sensors-interface-common.h>

G_BEGIN_DECLS

/**
 * Populate the application's main window
 * @param dialog: pointer to already filled sensors dialog structure
 * @return newly allocated GtkWidget of main window
 */
GtkWidget* create_main_window (t_sensors_dialog *dialog);

G_END_DECLS
