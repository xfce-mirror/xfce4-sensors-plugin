/* File: actions.h
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

#ifndef ACTIONS_H
#define ACTIONS_H

/* Glib includes */
#include <glib.h>

/* Package includes */
#include <sensors-interface-common.h>
#include <tacho.h>

/**
 * Shall refresh the entire view of the application
 * @param data: pointer to sensors dialog structure
 * @return TRUE: argument is valid
 */
void refresh_view (t_sensors_dialog *ptr_sensors_dialog);
gboolean refresh_view_cb (gpointer user_data);

#endif /* ACTIONS_H */
