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

/* cpu.c */

#include <math.h>
#include <gdk/gdk.h>
#include <glib/gprintf.h>

/* Package includes */
#include <cpu.h>
#include <types.h>

#define DEFAULT_HEIGHT 100
#define DEFAULT_WIDTH 100

#define min(a,b) a<b? a : b

/* forward declarations that are not published in the header
 * and only meant for internal access. */
static void gtk_cpu_class_init(GtkCpuClass *klass);
static void gtk_cpu_init(GtkCpu *cpu);
static void gtk_cpu_size_request(GtkWidget *widget,
    GtkRequisition *requisition);
static void gtk_cpu_size_allocate(GtkWidget *widget,
    GtkAllocation *allocation);
static void gtk_cpu_realize(GtkWidget *widget);
static gboolean gtk_cpu_expose(GtkWidget *widget,
    GdkEventExpose *event);
//static void gtk_cpu_paint(GtkWidget *widget);
static void gtk_cpu_destroy(GtkObject *object);


//extern gchar *font;
gchar *font;


GtkType
gtk_cpu_get_type(void)
{
  static GtkType gtk_cpu_type = 0;
  TRACE("enter gtk_cpu_get_type\n");

  if (!gtk_cpu_type) {
      static const GtkTypeInfo gtk_cpu_info = {
          "GtkCpu",
          sizeof(GtkCpu),
          sizeof(GtkCpuClass),
          (GtkClassInitFunc) gtk_cpu_class_init,
          (GtkObjectInitFunc) gtk_cpu_init,
          NULL,
          NULL,
          (GtkClassInitFunc) NULL
      };
      gtk_cpu_type = gtk_type_unique(GTK_TYPE_WIDGET, &gtk_cpu_info);
  }

  TRACE("leave gtk_cpu_get_type\n");
  return gtk_cpu_type;
}

/* this function implementation does not clean the drawable! */
/*
void
gtk_cpu_set_state (GtkCpu *cpu, gdouble num)
{
   cpu->sel = num;
   gtk_cpu_paint(GTK_WIDGET(cpu));
}
*/

void
gtk_cpu_set_text (GtkCpu *cpu, gchar *text)
{
  if (text==NULL) {
      gtk_cpu_unset_text(cpu);
      return;
  }
  else if (cpu->text!=NULL)
  {
    //g_free (cpu->text);
  }
    
  cpu->text = g_strdup(text);
  gdk_gc_destroy(cpu->gc);
  cpu->gc = NULL;
  gtk_cpu_paint(GTK_WIDGET(cpu));
}


void
gtk_cpu_unset_text (GtkCpu *cpu)
{
  if (cpu->text!=NULL)
    g_free (cpu->text);
    
  cpu->text = NULL;
  gdk_gc_destroy(cpu->gc);
  cpu->gc = NULL;
  gtk_cpu_paint(GTK_WIDGET(cpu));
}


GtkWidget * gtk_cpu_new(void)
{
  TRACE("enter gtk_cpu_new\n");
   return GTK_WIDGET(gtk_type_new(gtk_cpu_get_type()));
}


void gtk_cpu_set_color (GtkCpu *cpu, gchar *color)
{
  if (color==NULL) {
    gtk_cpu_unset_color(cpu);
    return;
  }
  else if (cpu->color!=NULL)
    g_free(cpu->color);
  
  cpu->color = g_strdup(color);
  gtk_cpu_paint(GTK_WIDGET(cpu));
}


void gtk_cpu_unset_color (GtkCpu *cpu)
{
  if (cpu->color!=NULL)
    g_free (cpu->color);
    
  cpu->color = g_strdup("#000000");
  gtk_cpu_paint(GTK_WIDGET(cpu));
}


static void
gtk_cpu_class_init (GtkCpuClass *klass)
{
  GtkWidgetClass *widget_class;
  GtkObjectClass *object_class;
  TRACE("enter gtk_cpu_class_init\n");


  widget_class = (GtkWidgetClass *) klass;
  object_class = (GtkObjectClass *) klass;

  widget_class->realize = gtk_cpu_realize;
  widget_class->size_request = gtk_cpu_size_request;
  widget_class->size_allocate = gtk_cpu_size_allocate;
  widget_class->expose_event = gtk_cpu_expose;

  object_class->destroy = gtk_cpu_destroy;
  
  font = g_strdup("Sans 12");
  TRACE("leave gtk_cpu_class_init\n");
}


static void
gtk_cpu_init (GtkCpu *cpu)
{
  TRACE("enter gtk_cpu_init\n");
  cpu->gc = NULL; /* can't allocate a valid GC because GtkWidget is not yet allocated */ 
  cpu->sel = 0.0;
  cpu->text = NULL;
  cpu->color = g_strdup("#000000");
  TRACE("leave gtk_cpu_init\n");
}


static void
gtk_cpu_size_request (GtkWidget *widget, GtkRequisition *requisition)
{
  TRACE("enter gtk_cpu_size_request\n");
  g_return_if_fail(widget != NULL);
  g_return_if_fail(GTK_IS_CPU(widget));
  g_return_if_fail(requisition != NULL);

  /* dynamic changes that scale the originally drawn picture accordingly */
  /* set the ratio, but actually this is not needed */
  requisition->width = DEFAULT_WIDTH;
  requisition->height = DEFAULT_HEIGHT;
  TRACE("leave gtk_cpu_size_request\n");
}


static void
gtk_cpu_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
  //int minwh;
  
  TRACE("enter gtk_cpu_size_allocate\n");
  DBG ("width x height = %d x %d\n", allocation->width, allocation->height);
  g_return_if_fail(widget != NULL);
  g_return_if_fail(GTK_IS_CPU(widget));
  g_return_if_fail(allocation != NULL);

  //minwh = min(allocation->width, allocation->height);
  //DBG("minimum is %d\n", minwh);
  
  allocation->width = allocation->height;
  //allocation->height = minwh;
  //DBG ("width x height = %d x %d\n", allocation->width, allocation->height);
  widget->allocation = *allocation;
  
  gtk_widget_set_size_request(widget, allocation->height, allocation->height);

  if (GTK_WIDGET_REALIZED(widget)) {
     gdk_window_move_resize(
         widget->window,
         allocation->x, allocation->y,
         allocation->height, allocation->height // determines width and height of the drawn area
     );
     //gtk_window_resize(widget->window, minwh, minwh);
  }
  DBG ("width x height = %d x %d\n", widget->allocation.width, widget->allocation.height);
  TRACE("leave gtk_cpu_size_allocate\n");
}


static void
gtk_cpu_realize (GtkWidget *widget)
{
  GdkWindowAttr attributes;
  guint attributes_mask;
  int minwh;
  TRACE("enter gtk_cpu_realize\n");

  g_return_if_fail(widget != NULL);
  g_return_if_fail(GTK_IS_CPU(widget));

  GTK_WIDGET_SET_FLAGS(widget, GTK_REALIZED);

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  
  /* define the minimum size; otherwise the area is not painted in the beginning */
  /* need square drawable area */
  minwh = min (widget->allocation.width, widget->allocation.height);
  attributes.width = minwh; // DEFAULT_WIDTH;
  attributes.height = minwh; //DEFAULT_HEIGHT;

  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.event_mask = gtk_widget_get_events(widget) | GDK_EXPOSURE_MASK;

  attributes_mask = GDK_WA_X | GDK_WA_Y;

  widget->window = gdk_window_new(
     gtk_widget_get_parent_window (widget),
     & attributes, attributes_mask
  );

  gdk_window_set_user_data(widget->window, widget);

  widget->style = gtk_style_attach(widget->style, widget->window);
  gtk_style_set_background(widget->style, widget->window, GTK_STATE_NORMAL);
  TRACE("leave gtk_cpu_realize\n");
}


static gboolean
gtk_cpu_expose(GtkWidget *widget, GdkEventExpose *event)
{
  TRACE("enter gtk_cpu_expose\n");
  DBG("event: %d\n", event->type);
  g_return_val_if_fail(widget != NULL, FALSE);
  g_return_val_if_fail(GTK_IS_CPU(widget), FALSE);
  g_return_val_if_fail(event != NULL, FALSE);

  gtk_cpu_paint(widget);

  TRACE("leave gtk_cpu_expose\n");
  return FALSE;
}


void
gtk_cpu_paint (GtkWidget *widget)
{
  gchar *text;
  GdkGC *gc;
  GdkColor *color;
  //GdkGCValues gcvalues;
  int i;
  double percent;
  PangoFontDescription *desc ;
  
  TRACE("enter gtk_cpu_paint\n");
  
  DBG ("Widget=0x%X, Window=0x%X\n", (unsigned int) widget, (unsigned int) widget->window);
  
  if (GTK_CPU(widget)->gc==NULL)
  {
    if (widget->window==NULL) /* safety checks to circumvent assertion failures when creating graphics contect */
      return;
      
    GTK_CPU(widget)->gc = gdk_gc_new(widget->window);
  }
  else // clear graphics context!
  {
    //gdk_gc_get_values(GDK_GC(GTK_CPU(widget)->gc), &gcvalues);
    //gcvalues.function = GDK_CLEAR;
    //gdk_draw_rectangle (widget->window,
                        //GTK_CPU(widget)->gc,
                        //TRUE,
                        //0, 0,
                        //widget->allocation.width, widget->allocation.height
    //);
    //gcvalues.function = GDK_NOOP
    gdk_window_clear(widget->window);
  }
  
  gc = GTK_CPU(widget)->gc;

  color = g_new0(GdkColor, 1);
  
  percent = GTK_CPU(widget)->sel; // *((double *) data);
  if (percent>1.0)
    percent = 1.0;
    
  //~ double d;
  DBG ("width x height = %d x %d\n", widget->allocation.width, widget->allocation.height);
  
  /* black border */
  gdk_gc_set_rgb_fg_color(gc, color);
  gdk_draw_arc (widget->window,
                gc, //drawable->style->fg_gc[GTK_STATE_PRELIGHT],
                TRUE,
                0, 0, 
                widget->allocation.width, widget->allocation.height,
               -45 * DEGREE_NORMALIZATION, THREE_QUARTER_CIRCLE * DEGREE_NORMALIZATION);

  /* white right part */
  color->red = 65535;
  color->green = 65535; //0.5 * COLOR_STEP*THREE_QUARTER_CIRCLE;
  color->blue = 65535;
  gdk_gc_set_rgb_fg_color(gc, color);
  gdk_draw_arc (widget->window,
                gc, //drawable->style->fg_gc[GTK_STATE_PRELIGHT],
                TRUE,
                1, 1, 
                widget->allocation.width-2, widget->allocation.height-2,
                -45 * DEGREE_NORMALIZATION, THREE_QUARTER_CIRCLE * DEGREE_NORMALIZATION); 
  
  /* define color for gradient */
  color->red = COLOR_STEP*THREE_QUARTER_CIRCLE;
  color->green = 0; //0.5 * COLOR_STEP*THREE_QUARTER_CIRCLE;
  color->blue = 16384;
  
  /* initialize color values appropriately */
  
  for (i=0; i<=(1-percent)*THREE_QUARTER_CIRCLE; i++)
  {
    if (i<(0.5*THREE_QUARTER_CIRCLE - 1))
      color->green += 2*COLOR_STEP;
    //else
      //color->green -= 0.5*COLOR_STEP;
      
    if (i>(0.5*THREE_QUARTER_CIRCLE  - 1))
      color->red -= 2*COLOR_STEP;
    //else 
      //color->red += 0.5*COLOR_STEP;
  }
  
  /* draw circular gradient */
  for (i=(1-percent)*THREE_QUARTER_CIRCLE; i<THREE_QUARTER_CIRCLE; i++)
  {
    
    gdk_gc_set_rgb_fg_color(gc, color);
    gdk_draw_arc (widget->window,
                  gc, //drawable->style->fg_gc[GTK_STATE_PRELIGHT],
                  TRUE,
                  1, 1, 
                  widget->allocation.width-2, widget->allocation.height-2,
                 (i-45) * DEGREE_NORMALIZATION, DEGREE_NORMALIZATION);
    
    if (i<(0.5*THREE_QUARTER_CIRCLE - 1))
      color->green += 2*COLOR_STEP;
    //else
      //color->green -= 0.5*COLOR_STEP;
      
    if (i>(0.5*THREE_QUARTER_CIRCLE - 1))
      color->red -= 2*COLOR_STEP;
    //else 
      //color->red += 0.5*COLOR_STEP;
      
  }
  
  /* draw closing black lines */
  color->green=0;
  color->red=0;
  color->blue=0;
  gdk_gc_set_rgb_fg_color(gc, color);
  gdk_draw_line (widget->window,
                gc, //drawable->style->fg_gc[GTK_STATE_PRELIGHT],
                0.5*widget->allocation.width, 0.5*widget->allocation.height, 
                0.5*(1+sin(M_PI_4))*widget->allocation.width, 0.5*(1+sin(M_PI_4))*widget->allocation.height);

  gdk_draw_line (widget->window,
                gc, //drawable->style->fg_gc[GTK_STATE_PRELIGHT],
                0.5*widget->allocation.width, 0.5*widget->allocation.height, 
                0.5*(1-sin(M_PI_4))*widget->allocation.width, 0.5*(1+sin(M_PI_4))*widget->allocation.height);
  
  if (GTK_CPU(widget)->text!=NULL)
  {

    PangoContext *context = gtk_widget_get_pango_context (widget); // pango_context_new ();
    PangoLayout *layout = pango_layout_new (context);
    
    pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);
    pango_layout_set_width (layout, widget->allocation.width);
    
    text = g_strdup_printf("<span color=\"%s\">%s</span>", GTK_CPU(widget)->color, GTK_CPU(widget)->text);
    pango_layout_set_markup (layout, text, -1); // with null-termination, no need to give length explicitly
        
    desc = pango_font_description_from_string(font); //pango_font_description_new ();
    if (desc==NULL)
      desc = pango_font_description_new ();
    pango_layout_set_font_description (layout, desc);
    
    gdk_draw_layout (widget->window, gc, 0.5*widget->allocation.width, 0.4*widget->allocation.height, layout);
    
    g_free (text);
    
  }
  
  g_free (color);
  TRACE("leave gtk_cpu_paint\n");
}


static void
gtk_cpu_destroy (GtkObject *object)
{
  GtkCpu *cpu;
  GtkCpuClass *klass;
  TRACE("enter gtk_cpu_destroy\n");

  g_return_if_fail(object != NULL);
  g_return_if_fail(GTK_IS_CPU(object));

  cpu = GTK_CPU(object);
  
  if (cpu->text!=NULL)
  {
    g_free (cpu->text);
    cpu->text = NULL;
  }
    
  if (cpu->color!=NULL)
  {
    g_free (cpu->color);
    cpu->color = NULL;
  }

  klass = gtk_type_class(gtk_widget_get_type());

  if (GTK_OBJECT_CLASS(klass)->destroy) {
     (* GTK_OBJECT_CLASS(klass)->destroy) (object);
  }
  TRACE("leave gtk_cpu_destroy\n");
}


void
gtk_cpu_set_value (GtkCpu *cpu, gdouble value)
{
  TRACE("enter gtk_cpu_set_value\n");
  //GdkRegion *region;
  if (value<0)
    value = 0.0;
  else if (value>1.0)
    value = 1.0;
  
  cpu->sel = value;

  //region = gdk_drawable_get_clip_region (GTK_WIDGET(cpu)->window);
  //gdk_window_invalidate_region (GTK_WIDGET(cpu)->window, region, TRUE);
  //gdk_window_process_updates (GTK_WIDGET(cpu)->window, TRUE);
  gtk_cpu_paint(GTK_WIDGET(cpu));
  TRACE("leave gtk_cpu_set_value\n");
}
