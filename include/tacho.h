/* File: tacho.h
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

#ifndef __TACHO_H
#define __TACHO_H

#include <cairo.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE (GtkSensorsTacho, gtk_sensorstacho, GTK, SENSORSTACHO, GtkDrawingArea)

typedef struct _GtkSensorsTacho GtkSensorsTacho;

/**
 * Enumeration of possible style for sensors tacho.
 */
enum SensorsTachoStyle {
    /** temperatures, ranging from green over yellow to red */
    style_MinGYR = 0,
    /** voltage and power, from yellow over green to blue */
    style_MediumYGB = 1,
    /** fill level, mostly used for energy from red over yellow to green */
    style_MaxRYG = 2
};

/**
 * pseudo widget for drawing a tacho
 */
struct _GtkSensorsTacho {
    /** real parent GtkDrawingArea widget */
    GtkDrawingArea parent;

    /** value to display */
    gdouble sel;

    /** pointer to string of sensor name to display */
    gchar *text;

    /** pointer to color as hexadecimal rgb value in a string */
    gchar *color_orNull;

    /** size of a side of the surrounding square area */
    guint size;

    /** color style of tacho */
    SensorsTachoStyle style;

    /** orientation, used for vertical bars */
    GtkOrientation orientation;
};

/**
 * set the value in percent that has to be visualized
 * @param tacho: Pointer to SensorsTacho structure
 * @param value: value in percent to visualize by the tacho display
 */
void gtk_sensorstacho_set_value (GtkSensorsTacho *tacho, gdouble value);

/**
 * create a new sensorstacho with orientation and initial size
 * @param orientation: orientation of the tacho
 * @param size: initial size of the tacho object
 * @param style: gradient style of the tacho object
 * @return allocated widget
 */
GtkWidget* gtk_sensorstacho_new(GtkOrientation orientation, guint size, SensorsTachoStyle style);

/**
 * set the text to be drawn. if NULL, no text is drawn.
 * @param tacho: Pointer to SensorsTacho structure
 * @param text: Text to set and paint
 */
void gtk_sensorstacho_set_text (GtkSensorsTacho *tacho, const gchar *text);

/**
 * Unset the text to be drawn. Will also free the internal copy of the text
 * @param tacho: Pointer to SensorsTacho structure
 */
void gtk_sensorstacho_unset_text (GtkSensorsTacho *tacho);

/** set the color of the text if any text is to be drawn at all
 * @param tacho: Pointer to SensorsTacho structure
 * @param color_orNull: Color in HTML notation, or NULL
 */
void gtk_sensorstacho_set_color (GtkSensorsTacho *tacho, const gchar *color_orNull);

/** Reset color to the default text color according to the current UI style.
 * @param tacho: Pointer to SensorsTacho structure
 */
void gtk_sensorstacho_unset_color (GtkSensorsTacho *tacho);

/**
 * Paint the SensorsTacho widget
 * @param widget: Pointer to SensorsTacho structure; hidden behind the widget interface
 * @param cr: pointer to cairo drawing structure
 */
gboolean gtk_sensorstacho_paint (GtkWidget *widget, cairo_t *cr);

/**
 * Set the new size to allocate for the widget and to fill with the painting
 * @param tacho: Pointer to SensorsTacho structure
 * @param size: The new size in both x and y direction to allocate and paint
 */
void gtk_sensorstacho_set_size(GtkSensorsTacho *tacho, guint size);

/**
 * directly exhibited internal string describing the currently set font for the
 * tacho elements.
 * TODO: Introduce getter/setter functions
 */
extern gchar *font;


#define MAX_HUE 0.8
#define ALPHA_CHANNEL_VALUE 0.8

/**
 * directly exhibited internal value describing the desired hue value for the
 * tacho elements.
 * TODO: Introduce getter/setter functions
 */
extern gfloat val_colorvalue;

/**
 * directly exhibited internal value describing the desired alpha value for the
 * tacho elements.
 * TODO: Introduce getter/setter functions
 */
extern gfloat val_alpha;

G_END_DECLS

#endif /* __TACHO_H */
