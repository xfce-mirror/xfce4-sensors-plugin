/* File: sensors-interface-types.h
 *
 * Copyright 2009-2017 Fabian Nowak (timystery@arcor.de)
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

#ifndef __SENSORS_INTERFACE_TYPES
#define __SENSORS_INTERFACE_TYPES

#include <glib.h>
#include <gtk/gtk.h>

/**
 * compound widget displaying a levelbar and optional label
 */
struct t_labelledlevelbar {
    /** the level bar */
    GtkWidget *progressbar;

    /** the label */
    GtkWidget *label;

    /** the surrounding box */
    GtkWidget *databox;

    /** a separate css provider per level-bar */
    char *css_data;
    GtkCssProvider *css_provider;
};


/**
 * enumeration of the available visualization styles for the Xfce 4 Panel Sensors Plugin
 */
enum e_displaystyles {
  DISPLAY_TEXT = 1,
  DISPLAY_BARS,
  DISPLAY_TACHO
};

#endif /* __SENSORS_INTERFACE_TYPES */
