/* File: tacho.h
 *
 *  Copyright 2009-2017 Fabian Nowak (timystery@arcor.de)
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

#ifndef __TACHO_H
#define __TACHO_H

#include <gtk/gtk.h>
#include <cairo.h>

G_BEGIN_DECLS


//#define GTK_TYPE_TACHO gtk_sensorstacho_get_type ()
//#define gtk_sensorstacho(obj) GTK_CHECK_CAST(obj, GTK_TYPE_CPU, GtkSensorsTacho)
//#define gtk_sensorstacho_CLASS(klass) GTK_CHECK_CLASS_CAST(klass, GTK_TYPE_CPU, GtkSensorsTachoClass)
//#define GTK_IS_CPU(obj) GTK_CHECK_TYPE(obj, GTK_TYPE_CPU)

//G_DEFINE_TYPE( GtkSensorsTacho, gtk_sensorstacho, GObject)
G_DECLARE_FINAL_TYPE (GtkSensorsTacho, gtk_sensorstacho, GTK, SENSORSTACHO, GtkDrawingArea)

//#define XFCE_TYPE_TACHO gtk_sensorstacho_get_type ()
//G_DECLARE_DERIVABLE_TYPE (XfceTacho, xfce_tacho, XFCE, TACHO, GObject)


//#define STROKER_TYPE_NODAL_CONTAINER                  (stroker_nodal_container_get_type ())
//#define STROKER_NODAL_CONTAINER(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), STROKER_TYPE_NODAL_CONTAINER, StrokerNodalContainer))
//#define STROKER_NODAL_CONTAINER_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST  ((klass), STROKER_TYPE_NODAL_CONTAINER, StrokerNodalContainerClass))
//#define STROKER_IS_NODAL_CONTAINER(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), STROKER_TYPE_NODAL_CONTAINER))
//#define STROKER_IS_NODAL_CONTAINER_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE  ((klass), STROKER_TYPE_NODAL_CONTAINER))
//#define STROKER_NODAL_CONTAINER_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS  ((obj), STROKER_TYPE_NODAL_CONTAINER, StrokerNodalContainerClass))

//#define gtk_sensorstacho(obj) G_TYPE_CHECK_INSTANCE_CAST(obj, gtk_sensorstacho_get_type (), GtkSensorsTacho)
//#define gtk_sensorstacho_CLASS(klass) G_TYPE_CHECK_CLASS_CAST(klass, gtk_sensorstacho_get_type(), GtkSensorsTachoClass)
//#define GTK_IS_CPU(obj) G_TYPE_CHECK_INSTANCE_TYPE(obj, gtk_sensorstacho_get_type())
//#define GTK_IS_CPU_CLASS(klass) G_TYPE_CHECK_CLASS_TYPE(klass, gtk_sensorstacho_get_type())
//#define gtk_sensorstacho_GET_CLASS(obj) G_TYPE_INSTANCE_GET_CLASS(obj, gtk_sensorstacho_get_type(), GtkSensorsTachoClass)




typedef struct _GtkSensorsTacho GtkSensorsTacho;
//typedef struct _GtkSensorsTachoClass GtkSensorsTachoClass;


struct _GtkSensorsTacho {
  GtkDrawingArea widget;
  //cairo_t *gc;
  gdouble sel;
  gchar *text;
  gchar *color;
  guint size;
  GtkOrientation orientation;
};

//struct _GtkSensorsTachoClass {
  //GtkDrawingAreaClass parent_class;
//};


//GType gtk_sensorstacho_get_type(void);
//void gtk_sensorstacho_set_sel(GtkSensorsTacho *tacho, gdouble sel);
void gtk_sensorstacho_set_value (GtkSensorsTacho *tacho, gdouble value);
GtkWidget * gtk_sensorstacho_new(GtkOrientation orientation, guint size);

/* set the text to be drawn. if NULL, no text is drawn */
void gtk_sensorstacho_set_text (GtkSensorsTacho *tacho, gchar *text);
void gtk_sensorstacho_unset_text (GtkSensorsTacho *tacho);

/* set and unset the color of the text if any text is to be drawn at all */
void gtk_sensorstacho_set_color (GtkSensorsTacho *tacho, gchar *color);
void gtk_sensorstacho_unset_color (GtkSensorsTacho *tacho);

gboolean gtk_sensorstacho_paint (GtkWidget *widget,
                cairo_t *ptr_cairo_context/*, gpointer data*/);

void gtk_sensorstacho_set_size(GtkSensorsTacho *tacho, guint size);

extern gchar *font;

G_END_DECLS

#endif /* __TACHO_H */
