/* $Id$ */
/*  Copyright 2009-2010 Fabian Nowak (timystery@arcor.de)
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

/* cpu.h */

#ifndef __CPU_H
#define __CPU_H

#include <gtk/gtk.h>
#include <cairo.h>

G_BEGIN_DECLS


#define GTK_CPU(obj) GTK_CHECK_CAST(obj, gtk_cpu_get_type (), GtkCpu)
#define GTK_CPU_CLASS(klass) GTK_CHECK_CLASS_CAST(klass, gtk_cpu_get_type(), GtkCpuClass)
#define GTK_IS_CPU(obj) GTK_CHECK_TYPE(obj, gtk_cpu_get_type())


#define SPACING 8
#define DEGREE_NORMALIZATION 64
#define THREE_QUARTER_CIRCLE 270
#define COLOR_STEP 220 // colors range from 0 to 2^16; we want 270 colors, hence 242

typedef struct _GtkCpu GtkCpu;
typedef struct _GtkCpuClass GtkCpuClass;


struct _GtkCpu {
  GtkWidget widget;
  GdkGC *gc;
  gdouble sel;
  gchar *text;
  gchar *color;
};

struct _GtkCpuClass {
  GtkWidgetClass parent_class;
};


GtkType gtk_cpu_get_type(void);
//void gtk_cpu_set_sel(GtkCpu *cpu, gdouble sel);
void gtk_cpu_set_value (GtkCpu *cpu, gdouble value);
GtkWidget * gtk_cpu_new();

/* set the text to be drawn. if NULL, no text is drawn */
void gtk_cpu_set_text (GtkCpu *cpu, gchar *text);
void gtk_cpu_unset_text (GtkCpu *cpu);

/* set and unset the color of the text if any text is to be drawn at all */
void gtk_cpu_set_color (GtkCpu *cpu, gchar *color);
void gtk_cpu_unset_color (GtkCpu *cpu);

void gtk_cpu_paint (GtkWidget *widget);

extern gchar *font;

G_END_DECLS

#endif /* __CPU_H */
