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

/* tacho.c */

#include <cairo.h>
#include <math.h>
#include <gdk/gdk.h>
#include <glib/gprintf.h>
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

/* forward declarations that are not published in the header
 * and only meant for internal access. */
static void gtk_sensorstacho_get_preferred_width_for_height(GtkWidget *widget,
                                        gint      height,
                                        gint      *minimal_width,
                                        gint      *natural_width);
static void gtk_sensorstacho_get_preferred_height_for_width(GtkWidget *widget,
                                         gint      width,
                                         gint      *minimal_height,
                                         gint      *natural_height);
static void gtk_sensorstacho_get_preferred_width(GtkWidget *widget,
                                        gint      *minimal_width,
                                        gint      *natural_width);
static void gtk_sensorstacho_get_preferred_height(GtkWidget *widget,
                                         gint      *minimal_height,
                                         gint      *natural_height);
static void gtk_sensorstacho_size_allocate(GtkWidget *widget,
                                  GtkAllocation *allocation);
static GtkSizeRequestMode gtk_sensorstacho_get_request_mode(GtkWidget *widget);

void gtk_sensorstacho_destroy(GtkWidget *widget);

gchar *font = NULL; // declared as extern in tacho.h

gfloat val_colorvalue = MAX_HUE; // declared as extern in tacho.h
gfloat val_alpha = ALPHA_CHANNEL_VALUE; // declared as extern in tacho.h

G_DEFINE_TYPE( GtkSensorsTacho, gtk_sensorstacho, GTK_TYPE_DRAWING_AREA )


/* -------------------------------------------------------------------------- */
void
gtk_sensorstacho_set_text (GtkSensorsTacho *ptr_sensorstacho, gchar *ptr_text)
{
    g_return_if_fail (ptr_sensorstacho != NULL);

    gtk_sensorstacho_unset_text(ptr_sensorstacho);

    if (ptr_text != NULL) {
        ptr_sensorstacho->text = g_strdup(ptr_text);
    }
}

/* -------------------------------------------------------------------------- */
void
gtk_sensorstacho_unset_text (GtkSensorsTacho *ptr_sensorstacho)
{
    g_return_if_fail (ptr_sensorstacho != NULL);

    if (ptr_sensorstacho->text != NULL)
        g_free (ptr_sensorstacho->text);

    ptr_sensorstacho->text = NULL;
}

/* -------------------------------------------------------------------------- */
GtkWidget *
gtk_sensorstacho_new(GtkOrientation orientation, guint size, SensorsTachoStyle style)
{
    GtkSensorsTacho *ptr_sensorstacho = g_object_new(gtk_sensorstacho_get_type(), NULL);

    ptr_sensorstacho->orientation = orientation;
    ptr_sensorstacho->size = size;
    ptr_sensorstacho->style = style;

    return GTK_WIDGET(ptr_sensorstacho);
}

/* -------------------------------------------------------------------------- */
void
gtk_sensorstacho_set_color (GtkSensorsTacho *ptr_sensorstacho, gchar *color)
{
    g_return_if_fail (ptr_sensorstacho != NULL);

    if (color == NULL) {
        color = "#000000";
    }

    if (ptr_sensorstacho->color != NULL)
        g_free(ptr_sensorstacho->color);

    ptr_sensorstacho->color = g_strdup(color);
}

/* -------------------------------------------------------------------------- */
static void
gtk_sensorstacho_class_init (GtkSensorsTachoClass *ptr_gtksensorstachoclass)
{
    GtkWidgetClass *ptr_widgetclass;

    ptr_widgetclass = GTK_WIDGET_CLASS (ptr_gtksensorstachoclass);

    ptr_widgetclass->get_request_mode = gtk_sensorstacho_get_request_mode;
    ptr_widgetclass->get_preferred_width = gtk_sensorstacho_get_preferred_width;
    ptr_widgetclass->get_preferred_height = gtk_sensorstacho_get_preferred_height;
    ptr_widgetclass->get_preferred_width_for_height = gtk_sensorstacho_get_preferred_width_for_height;
    ptr_widgetclass->get_preferred_height_for_width = gtk_sensorstacho_get_preferred_height_for_width;
    ptr_widgetclass->size_allocate = gtk_sensorstacho_size_allocate;
    ptr_widgetclass->draw = gtk_sensorstacho_paint;
    ptr_widgetclass->destroy = gtk_sensorstacho_destroy;

    if (font==NULL)
        font = g_strdup("Sans 11");
}

/* -------------------------------------------------------------------------- */
static void
gtk_sensorstacho_init (GtkSensorsTacho *ptr_sensorstacho)
{
    g_return_if_fail (ptr_sensorstacho != NULL);

    ptr_sensorstacho->sel = 0.0;

    gtk_sensorstacho_unset_text(ptr_sensorstacho);

    gtk_sensorstacho_set_color(ptr_sensorstacho, NULL);
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
gtk_sensorstacho_get_preferred_width(GtkWidget *ptr_widget,
                                    gint      *ptr_minimalwidth,
                                    gint      *ptr_naturalwidth)
{
    g_return_if_fail (ptr_widget != NULL);

    if (ptr_minimalwidth != NULL)
        *ptr_minimalwidth = MAX(GTK_SENSORSTACHO(ptr_widget)->size, MINIMUM_WIDTH);

    if (ptr_naturalwidth != NULL)
        *ptr_naturalwidth = GTK_SENSORSTACHO(ptr_widget)->size;

    DBG("Returning widget preferred width: %d, %d.\n", *ptr_minimalwidth, *ptr_naturalwidth);
}

/* -------------------------------------------------------------------------- */
static void
gtk_sensorstacho_get_preferred_height(GtkWidget *ptr_widget,
                                gint      *ptr_minimalheight,
                                gint      *ptr_naturalheight)
{
    g_return_if_fail (ptr_widget != NULL);

    if (ptr_minimalheight)
        *ptr_minimalheight = MAX(GTK_SENSORSTACHO(ptr_widget)->size, MINIMUM_HEIGHT);

    if (ptr_naturalheight)
        *ptr_naturalheight = GTK_SENSORSTACHO(ptr_widget)->size;

    DBG("Returning widget preferred height: %d, %d.\n", *ptr_minimalheight, *ptr_naturalheight);
}

/* -------------------------------------------------------------------------- */
static void
gtk_sensorstacho_get_preferred_width_for_height(GtkWidget *ptr_widget,
                                gint             height,
                                gint      *ptr_minimalwidth,
                                gint      *ptr_naturalwidth)
{
    g_return_if_fail(ptr_widget != NULL);

    *ptr_minimalwidth =
    *ptr_naturalwidth = MAX(height, MINIMUM_WIDTH);
    DBG ("Returning preferred natural width %d for height %d.\n", *ptr_naturalwidth, height);
}

/* -------------------------------------------------------------------------- */
static void
gtk_sensorstacho_get_preferred_height_for_width(GtkWidget *ptr_widget,
                                gint      width,
                                gint      *ptr_minimalheight,
                                gint      *ptr_naturalheight)
{
    g_return_if_fail(ptr_widget != NULL);

    *ptr_minimalheight =
    *ptr_naturalheight = MAX(width, MINIMUM_HEIGHT);
    DBG ("Returning preferred natural height %d for width %d.\n", *ptr_naturalheight, width);
}

/* -------------------------------------------------------------------------- */
static void
gtk_sensorstacho_size_allocate (GtkWidget *ptr_widget, GtkAllocation *ptr_allocation)
{
  gint min_widthheight;

  g_return_if_fail(ptr_widget != NULL);
  g_return_if_fail(GTK_IS_SENSORSTACHO(ptr_widget));
  g_return_if_fail(ptr_allocation != NULL);

  min_widthheight =
  ptr_allocation->width = ptr_allocation->height = MIN(ptr_allocation->width, ptr_allocation->height);
  gtk_widget_set_allocation(ptr_widget, ptr_allocation); // this seems to be the main thing of the current function

  gtk_widget_set_size_request(ptr_widget, min_widthheight, min_widthheight);

  if (gtk_widget_get_realized(ptr_widget))
  {
     gdk_window_move(
         gtk_widget_get_window(ptr_widget),
         ptr_allocation->x, ptr_allocation->y
     );
  }
}


/* -------------------------------------------------------------------------- */
gboolean
gtk_sensorstacho_paint (GtkWidget *widget,
                        cairo_t *ptr_cairo)
{
    gchar *ptr_text = NULL;
    GdkRGBA color;
    int i = 0;
    double percent = 0.0;
    PangoFontDescription *ptr_pangofontdescription = NULL;
    gint width, height;
    gint pos_xcenter, pos_ycenter;
    double degrees_135 = (135) * G_PI / 180;
    double degrees_45minusI;
    GtkAllocation allocation;
    GtkStyleContext *ptr_stylecontext = NULL;
    GtkSensorsTacho *ptr_tacho = GTK_SENSORSTACHO(widget);

    g_return_val_if_fail (ptr_cairo!=NULL, FALSE);

    gtk_widget_get_allocation(widget, &allocation);

    percent = ptr_tacho->sel;
    if (percent>1.0)
        percent = 1.0;

    width = gtk_widget_get_allocated_width (widget);
    height = gtk_widget_get_allocated_height (widget);

    cairo_reset_clip(ptr_cairo);

    width = height = MIN(width, height);

    pos_xcenter = width / 2;
    pos_ycenter = height / 2;

    /* initialize color values appropriately */
    color.red = (ptr_tacho->style != style_MediumYGB) ? val_colorvalue : 0;
    color.green = val_colorvalue;
    color.blue = 0;
    color.alpha = val_alpha;

    if (percent < 0.5)
    {
        if (ptr_tacho->style == style_MinGYR)
            color.red = 2*val_colorvalue * percent;
        else if (ptr_tacho->style == style_MaxRYG)
            color.green = 2*val_colorvalue * percent;
        else
            color.red = 2*val_colorvalue * (0.5 - percent);
    }

    if (percent > 0.5)
    {
        if (ptr_tacho->style == style_MinGYR)
            color.green = 2*val_colorvalue * (1 - percent);
        else if (ptr_tacho->style == style_MaxRYG)
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
        gdk_cairo_set_source_rgba (ptr_cairo, &color);

        degrees_45minusI = (45-i) * G_PI / 180;

        cairo_arc (ptr_cairo,
           pos_xcenter, pos_ycenter,
           width/2-2,
           degrees_135,
           degrees_45minusI);

        cairo_line_to (ptr_cairo, pos_xcenter, pos_ycenter);

        cairo_arc (ptr_cairo, pos_xcenter, pos_ycenter, width/2-4, degrees_45minusI, degrees_45minusI);
        cairo_line_to (ptr_cairo, pos_xcenter, pos_ycenter);

        cairo_fill (ptr_cairo);

        if (i>(0.5*THREE_QUARTER_CIRCLE - 1))
        {
            if (ptr_tacho->style == style_MinGYR)
                color.red -= 2*val_colorvalue * COLOR_STEP;
            else if (ptr_tacho->style == style_MaxRYG)
                color.green -= 2*val_colorvalue * COLOR_STEP;
            else {
                color.red += 2*val_colorvalue * COLOR_STEP;
            }
        }

        if (i<(0.5*THREE_QUARTER_CIRCLE - 1))
        {
            if (ptr_tacho->style == style_MinGYR)
                color.green += 2*val_colorvalue * COLOR_STEP;
            else if (ptr_tacho->style == style_MaxRYG)
                color.red += 2*val_colorvalue * COLOR_STEP;
            else {
                color.green += 2*val_colorvalue * COLOR_STEP;
                color.blue -= 2*val_colorvalue * COLOR_STEP;
            }
        }
    }

    /* white right part */
    cairo_arc (ptr_cairo,
               pos_xcenter, pos_ycenter,
               width/2-2,
               degrees_135, 45 * G_PI / 180);

    cairo_line_to (ptr_cairo, pos_xcenter, pos_ycenter);

    cairo_arc (ptr_cairo, pos_xcenter, pos_ycenter, width/2-2, degrees_135, degrees_135);
    cairo_line_to (ptr_cairo, pos_xcenter, pos_ycenter);

    /* black border */
    cairo_set_line_width (ptr_cairo, 0.5);

    ptr_stylecontext = gtk_widget_get_style_context(widget);
    if (ptr_stylecontext != NULL)
        gtk_style_context_get_color(ptr_stylecontext, GTK_STATE_FLAG_NORMAL, &color);
    else
    {
        color.red = 0.0f;
        color.green = 0.0f;
        color.blue = 0.0f;
    }

    gdk_cairo_set_source_rgba (ptr_cairo, &color);

    cairo_stroke (ptr_cairo);

    if (ptr_tacho->text != NULL) {
        PangoContext *ptr_style_context = gtk_widget_get_pango_context (widget);
        PangoLayout *ptr_layout = pango_layout_new (ptr_style_context);

        pango_layout_set_alignment(ptr_layout, PANGO_ALIGN_CENTER);
        pango_layout_set_width (ptr_layout, width);

        ptr_text = g_strdup_printf("<span color=\"%s\">%s</span>", ptr_tacho->color, ptr_tacho->text);
        pango_layout_set_markup (ptr_layout, ptr_text, -1); // with null-termination, no need to give length explicitly
        g_free(ptr_text);

        ptr_pangofontdescription = pango_font_description_from_string(font);
        pango_layout_set_font_description (ptr_layout, ptr_pangofontdescription);
        pango_font_description_free (ptr_pangofontdescription);

        pango_cairo_update_layout (ptr_cairo, ptr_layout);

        pango_layout_get_size (ptr_layout, &width, &height);

        cairo_move_to (ptr_cairo, pos_xcenter, pos_ycenter - height / 2 / PANGO_SCALE);
        pango_cairo_show_layout (ptr_cairo, ptr_layout);
        g_object_unref(ptr_layout);
    }

    return TRUE;
}


/* -------------------------------------------------------------------------- */
void
gtk_sensorstacho_destroy(GtkWidget *widget)
{
    GtkSensorsTacho *ptr_sensorstacho = GTK_SENSORSTACHO(widget);

    g_return_if_fail(ptr_sensorstacho!=NULL);

    if (ptr_sensorstacho->color != NULL)
    {
        g_free(ptr_sensorstacho->color);
        ptr_sensorstacho->color = NULL;
    }

    gtk_sensorstacho_unset_text(ptr_sensorstacho);
}


/* -------------------------------------------------------------------------- */
void
gtk_sensorstacho_set_value (GtkSensorsTacho *ptr_sensorstacho, gdouble value)
{
    if (value < 0)
        value = 0.0;
    else if (value > 1.0)
        value = 1.0;

    g_return_if_fail (ptr_sensorstacho != NULL);

    ptr_sensorstacho->sel = value;
}

/* -------------------------------------------------------------------------- */
void
gtk_sensorstacho_set_size(GtkSensorsTacho *ptr_sensorstacho, guint size)
{
    g_return_if_fail (ptr_sensorstacho != NULL);
    ptr_sensorstacho->size = size;
}
