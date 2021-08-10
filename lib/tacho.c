/* File: tacho.c
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

#include <cairo.h>
#include <gdk/gdk.h>
#include <glib/gprintf.h>
#include <math.h>
#include <stdlib.h>

/* Package includes */
#include <tacho.h>
#include <types.h>

#define DEFAULT_HEIGHT 64
#define DEFAULT_WIDTH 64

#define MINIMUM_WIDTH 12
#define MINIMUM_HEIGHT 12

#define SPACING 8
#define DEGREE_NORMALIZATION 64
#define THREE_QUARTER_CIRCLE 270
#define COLOR_STEP 1.0/THREE_QUARTER_CIRCLE // colors range from 0 to 2^16; we want 270 colors, hence 242

static void
gtk_sensorstacho_get_preferred_width_for_height(GtkWidget *widget,
                                                gint       height,
                                                gint      *minimal_width,
                                                gint      *natural_width);

static void
gtk_sensorstacho_get_preferred_height_for_width(GtkWidget *widget,
                                                gint       width,
                                                gint      *minimal_height,
                                                gint      *natural_height);

static void
gtk_sensorstacho_get_preferred_width(GtkWidget *widget,
                                     gint      *minimal_width,
                                     gint      *natural_width);

static void
gtk_sensorstacho_get_preferred_height(GtkWidget *widget,
                                      gint      *minimal_height,
                                      gint      *natural_height);

static GtkSizeRequestMode
gtk_sensorstacho_get_request_mode(GtkWidget *widget);

void gtk_sensorstacho_destroy(GtkWidget *widget);

gchar *font = NULL; // declared as extern in tacho.h

gfloat val_colorvalue = MAX_HUE; // declared as extern in tacho.h
gfloat val_alpha = ALPHA_CHANNEL_VALUE; // declared as extern in tacho.h

G_DEFINE_TYPE( GtkSensorsTacho, gtk_sensorstacho, GTK_TYPE_DRAWING_AREA )


/* -------------------------------------------------------------------------- */
void
gtk_sensorstacho_set_text (GtkSensorsTacho *tacho, const gchar *text)
{
    g_return_if_fail (tacho != NULL);

    gtk_sensorstacho_unset_text (tacho);
    if (text)
        tacho->text = g_strdup (text);
}

/* -------------------------------------------------------------------------- */
void
gtk_sensorstacho_unset_text (GtkSensorsTacho *tacho)
{
    g_return_if_fail (tacho != NULL);

    g_free (tacho->text);
    tacho->text = NULL;
}

/* -------------------------------------------------------------------------- */
GtkWidget *
gtk_sensorstacho_new(GtkOrientation orientation, guint size, SensorsTachoStyle style)
{
    GtkSensorsTacho *tacho = g_object_new (gtk_sensorstacho_get_type(), NULL);

    tacho->orientation = orientation;
    tacho->size = size;
    tacho->style = style;

    return GTK_WIDGET(tacho);
}

/* -------------------------------------------------------------------------- */
void
gtk_sensorstacho_set_color (GtkSensorsTacho *tacho, const gchar *color_orNull)
{
    g_return_if_fail (tacho != NULL);

    gtk_sensorstacho_unset_color (tacho);
    if (color_orNull && *color_orNull != '\0')
        tacho->color_orNull = g_strdup(color_orNull);
}

/* -------------------------------------------------------------------------- */
void
gtk_sensorstacho_unset_color (GtkSensorsTacho *tacho)
{
    g_return_if_fail (tacho != NULL);

    g_free (tacho->color_orNull);
    tacho->color_orNull = NULL;
}

/* -------------------------------------------------------------------------- */
static void
gtk_sensorstacho_class_init (GtkSensorsTachoClass *tacho_class)
{
    GtkWidgetClass *widget_class;

    widget_class = GTK_WIDGET_CLASS (tacho_class);

    widget_class->get_request_mode = gtk_sensorstacho_get_request_mode;
    widget_class->get_preferred_width = gtk_sensorstacho_get_preferred_width;
    widget_class->get_preferred_height = gtk_sensorstacho_get_preferred_height;
    widget_class->get_preferred_width_for_height = gtk_sensorstacho_get_preferred_width_for_height;
    widget_class->get_preferred_height_for_width = gtk_sensorstacho_get_preferred_height_for_width;
    widget_class->draw = gtk_sensorstacho_paint;
    widget_class->destroy = gtk_sensorstacho_destroy;

    if (font == NULL)
        font = g_strdup("Sans 11");
}

/* -------------------------------------------------------------------------- */
static void
gtk_sensorstacho_init (GtkSensorsTacho *tacho)
{
    g_return_if_fail (tacho != NULL);

    tacho->sel = 0.0;
    gtk_sensorstacho_unset_text (tacho);
    gtk_sensorstacho_set_color( tacho, NULL);
}

/* -------------------------------------------------------------------------- */
static GtkSizeRequestMode
gtk_sensorstacho_get_request_mode(GtkWidget *widget)
{
    GtkSizeRequestMode res_requestmode = GTK_SIZE_REQUEST_WIDTH_FOR_HEIGHT;
    g_return_val_if_fail(widget != NULL, GTK_SIZE_REQUEST_CONSTANT_SIZE);

    if (GTK_SENSORSTACHO(widget)->orientation == GTK_ORIENTATION_VERTICAL)
        res_requestmode = GTK_SIZE_REQUEST_HEIGHT_FOR_WIDTH;

    DBG ("Getting preferred mode: %d.\n", res_requestmode);
    return res_requestmode;
}

/* -------------------------------------------------------------------------- */
static void
gtk_sensorstacho_get_preferred_width(GtkWidget *widget,
                                     gint      *minimal_width,
                                     gint      *natural_width)
{
    g_return_if_fail (widget != NULL);

    if (minimal_width != NULL)
        *minimal_width = MAX(GTK_SENSORSTACHO(widget)->size, MINIMUM_WIDTH);

    if (natural_width != NULL)
        *natural_width = GTK_SENSORSTACHO(widget)->size;

    if ((minimal_width != NULL) && (natural_width != NULL))
        DBG("Returning widget preferred width: %d, %d.\n", *minimal_width, *natural_width);
}

/* -------------------------------------------------------------------------- */
static void
gtk_sensorstacho_get_preferred_height(GtkWidget *widget,
                                      gint      *minimal_height,
                                      gint      *natural_height)
{
    g_return_if_fail (widget != NULL);

    if (minimal_height)
        *minimal_height = MAX(GTK_SENSORSTACHO(widget)->size, MINIMUM_HEIGHT);

    if (natural_height)
        *natural_height = GTK_SENSORSTACHO(widget)->size;

    if ((minimal_height != NULL) && (natural_height != NULL))
        DBG("Returning widget preferred height: %d, %d.\n", *minimal_height, *natural_height);
}

/* -------------------------------------------------------------------------- */
static void
gtk_sensorstacho_get_preferred_width_for_height(GtkWidget *widget,
                                                gint       height,
                                                gint      *minimal_width,
                                                gint      *natural_width)
{
    g_return_if_fail(widget != NULL);

    *minimal_width =
    *natural_width = MAX(height, MINIMUM_WIDTH);
    DBG ("Returning preferred natural width %d for height %d.\n", *natural_width, height);
}

/* -------------------------------------------------------------------------- */
static void
gtk_sensorstacho_get_preferred_height_for_width(GtkWidget *widget,
                                                gint       width,
                                                gint      *minimal_height,
                                                gint      *natural_height)
{
    g_return_if_fail(widget != NULL);

    *minimal_height =
    *natural_height = MAX(width, MINIMUM_HEIGHT);
    DBG ("Returning preferred natural height %d for width %d.\n", *natural_height, width);
}

/* -------------------------------------------------------------------------- */
gboolean
gtk_sensorstacho_paint (GtkWidget *widget, cairo_t *cr)
{
    GdkRGBA color;
    int i;
    double percent = 0;
    gint width, height;
    gint pos_xcenter, pos_ycenter;
    const double degrees_135 = 135 * G_PI / 180;
    GtkAllocation allocation;
    GtkStyleContext *style_context = NULL;
    GtkSensorsTacho *tacho = GTK_SENSORSTACHO (widget);

    g_return_val_if_fail (cr != NULL, FALSE);

    gtk_widget_get_allocation(widget, &allocation);

    percent = tacho->sel;
    if (G_UNLIKELY (isnan (percent)))
        percent = 0;
    else if (G_UNLIKELY (percent < 0))
        percent = 0;
    else if (G_UNLIKELY (percent > 1))
        percent = 1;

    width = gtk_widget_get_allocated_width (widget);
    height = gtk_widget_get_allocated_height (widget);
    width = height = MIN(width, height);

    cairo_reset_clip(cr);

    pos_xcenter = width / 2;
    pos_ycenter = height / 2;
    pos_ycenter += height * (1 - fabs (sin (degrees_135))) / 4;  /* center the arc vertically */

    /* initialize color values appropriately */
    color.red = (tacho->style != style_MediumYGB) ? val_colorvalue : 0;
    color.green = val_colorvalue;
    color.blue = 0;
    color.alpha = val_alpha;

    if (percent < 0.5)
    {
        if (tacho->style == style_MinGYR)
            color.red = 2*val_colorvalue * percent;
        else if (tacho->style == style_MaxRYG)
            color.green = 2*val_colorvalue * percent;
        else
            color.red = 2*val_colorvalue * (0.5 - percent);
    }

    if (percent > 0.5)
    {
        if (tacho->style == style_MinGYR)
            color.green = 2*val_colorvalue * (1 - percent);
        else if (tacho->style == style_MaxRYG)
            color.red = 2*val_colorvalue * (1 - percent);
        else
        {
            color.green = 2*val_colorvalue * (1.0 - percent);
            color.blue = 2*val_colorvalue * (percent - 0.5);
        }
    }

    /* draw circular gradient */
    for (i=(1-percent)*THREE_QUARTER_CIRCLE; i<THREE_QUARTER_CIRCLE; i++)
    {
        const double degrees_45minusI = (45-i) * G_PI / 180;

        gdk_cairo_set_source_rgba (cr, &color);

        cairo_arc (cr, pos_xcenter, pos_ycenter, width/2-2, degrees_135, degrees_45minusI);
        cairo_line_to (cr, pos_xcenter, pos_ycenter);
        cairo_arc (cr, pos_xcenter, pos_ycenter, width/2-4, degrees_45minusI, degrees_45minusI);
        cairo_line_to (cr, pos_xcenter, pos_ycenter);
        cairo_fill (cr);

        if (i > (0.5*THREE_QUARTER_CIRCLE - 1))
        {
            if (tacho->style == style_MinGYR)
                color.red -= 2*val_colorvalue * COLOR_STEP;
            else if (tacho->style == style_MaxRYG)
                color.green -= 2*val_colorvalue * COLOR_STEP;
            else {
                color.red += 2*val_colorvalue * COLOR_STEP;
            }
        }

        if (i < (0.5*THREE_QUARTER_CIRCLE - 1))
        {
            if (tacho->style == style_MinGYR)
                color.green += 2*val_colorvalue * COLOR_STEP;
            else if (tacho->style == style_MaxRYG)
                color.red += 2*val_colorvalue * COLOR_STEP;
            else {
                color.green += 2*val_colorvalue * COLOR_STEP;
                color.blue -= 2*val_colorvalue * COLOR_STEP;
            }
        }
    }

    /* white right part */
    cairo_arc (cr, pos_xcenter, pos_ycenter, width/2-2, degrees_135, 45 * G_PI / 180);

    cairo_line_to (cr, pos_xcenter, pos_ycenter);

    cairo_arc (cr, pos_xcenter, pos_ycenter, width/2-2, degrees_135, degrees_135);
    cairo_line_to (cr, pos_xcenter, pos_ycenter);

    /* black border */
    cairo_set_line_width (cr, 0.5);

    style_context = gtk_widget_get_style_context(widget);
    if (style_context != NULL)
        gtk_style_context_get_color(style_context, GTK_STATE_FLAG_NORMAL, &color);
    else
    {
        color.red = 0.0f;
        color.green = 0.0f;
        color.blue = 0.0f;
    }

    gdk_cairo_set_source_rgba (cr, &color);

    cairo_stroke (cr);

    if (tacho->text != NULL) {
        PangoContext *style_context1 = gtk_widget_get_pango_context (widget);
        PangoLayout *layout = pango_layout_new (style_context1);
        gchar *text;
        PangoFontDescription *pango_font_description;
        gdouble baseline;

        pango_layout_set_alignment (layout, PANGO_ALIGN_CENTER);
        pango_layout_set_width (layout, width);

        if (tacho->color_orNull && *tacho->color_orNull != '\0')
            text = g_strdup_printf ("<span color=\"%s\">%s</span>", tacho->color_orNull, tacho->text);
        else
            text = g_strdup_printf ("<span>%s</span>", tacho->text);
        pango_layout_set_markup (layout, text, -1);
        g_free(text);

        pango_font_description = pango_font_description_from_string(font);
        pango_layout_set_font_description (layout, pango_font_description);
        pango_font_description_free (pango_font_description);

        pango_cairo_update_layout (cr, layout);

        baseline = pango_layout_get_baseline (layout);

        cairo_move_to (cr, pos_xcenter, pos_ycenter - baseline / PANGO_SCALE - 1);
        pango_cairo_show_layout (cr, layout);
        g_object_unref(layout);
    }

    return TRUE;
}


/* -------------------------------------------------------------------------- */
void
gtk_sensorstacho_destroy(GtkWidget *widget)
{
    GtkSensorsTacho *tacho = GTK_SENSORSTACHO(widget);

    g_return_if_fail(tacho!=NULL);

    if (tacho->color_orNull)
    {
        g_free(tacho->color_orNull);
        tacho->color_orNull = NULL;
    }

    gtk_sensorstacho_unset_color(tacho);
    gtk_sensorstacho_unset_text(tacho);
}


/* -------------------------------------------------------------------------- */
void
gtk_sensorstacho_set_value (GtkSensorsTacho *tacho, gdouble value)
{
    g_return_if_fail (tacho != NULL);

    if (G_UNLIKELY (isnan (value)))
        value = 0;
    else if (G_UNLIKELY (value < 0))
        value = 0;
    else if (G_UNLIKELY (value > 1))
        value = 1;

    tacho->sel = value;
}

/* -------------------------------------------------------------------------- */
void
gtk_sensorstacho_set_size(GtkSensorsTacho *tacho, guint size)
{
    g_return_if_fail (tacho != NULL);
    if (tacho->size != size)
    {
        tacho->size = size;
        gtk_widget_queue_resize (GTK_WIDGET(tacho));
    }
}
