/*  Copyright 2009-2017 Fabian Nowak (timystery@arcor.de)
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

/* tacho.c */

#include <cairo.h>
#include <math.h>
#include <gdk/gdk.h>
#include <glib/gprintf.h>

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

//#define min(a,b) a<b? a : b

/* forward declarations that are not published in the header
 * and only meant for internal access. */
//static void gtk_sensorstacho_class_init(GtkSensorsTachoClass *klass);
//static void gtk_sensorstacho_init(GtkSensorsTacho *cpu);
//static void gtk_sensorstacho_size_request (GtkWidget *widget,
                                  //GtkRequisition *requisition);
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
//static void gtk_sensorstacho_realize(GtkWidget *widget);
//static gboolean gtk_sensorstacho_expose(GtkWidget *widget,
    //GdkEventExpose *event);
//static void gtk_sensorstacho_destroy(GtkObject *object);
//static gboolean gtk_sensorstacho_button_press (GtkWidget      *widget,
                       //GdkEventButton *event);


gchar *font = NULL; // declared as extern in tacho.h

G_DEFINE_TYPE( GtkSensorsTacho, gtk_sensorstacho, GTK_TYPE_DRAWING_AREA )

//GType
//gtk_sensorstacho_get_type(void)
//{
  //static GType gtk_sensorstacho_type = 0;
  //TRACE("enter gtk_sensorstacho_get_type\n");

  //if (!gtk_sensorstacho_type) {
      //static const GtkTypeInfo gtk_sensorstacho_info = {
          //"GtkSensorsTacho",
          //sizeof(GtkSensorsTacho),
          //sizeof(GtkSensorsTachoClass),
          //(GtkClassInitFunc) gtk_sensorstacho_class_init,
          //(GtkObjectInitFunc) gtk_sensorstacho_init,
          //NULL,
          //NULL,
          //(GtkClassInitFunc) NULL
      //};
      //gtk_sensorstacho_type = gtk_type_unique(GTK_TYPE_WIDGET, &gtk_sensorstacho_info);
  //}

  //TRACE("leave gtk_sensorstacho_get_type\n");
  //return gtk_sensorstacho_type;
//}

/* this function implementation does not clean the drawable! */
/*
void
gtk_sensorstacho_set_state (GtkSensorsTacho *cpu, gdouble num)
{
   cpu->sel = num;
   gtk_sensorstacho_paint(GTK_WIDGET(cpu));
}*/


void
gtk_sensorstacho_set_text (GtkSensorsTacho *ptr_sensorstacho, gchar *text)
{
    cairo_t *ptr_context = NULL;
    GdkWindow *ptr_gdkwindowsensorstacho = NULL;
    //GdkDrawingContext *ptr_gdkdrawingcontext = NULL;

    g_return_if_fail (ptr_sensorstacho != NULL);

    if (text==NULL) {
        gtk_sensorstacho_unset_text(ptr_sensorstacho);
        return;
    }

    ptr_sensorstacho->text = g_strdup(text);
    //ptr_sensorstacho->gc = NULL;
    ptr_gdkwindowsensorstacho = gtk_widget_get_window(GTK_WIDGET(ptr_sensorstacho));

    if (ptr_gdkwindowsensorstacho != NULL)
    {
        //ptr_context = gdk_cairo_create (ptr_gdkwindowsensorstacho);
         //ptr_gdkdrawingcontext = gdk_window_begin_draw_frame(ptr_gdkwindowsensorstacho, region);
         //ptr_context = gdk_drawing_context_get_cairo_context(ptr_gdkdrawingcontext);
    }

    if (ptr_context != NULL) {
        //gtk_sensorstacho_paint(GTK_WIDGET(ptr_sensorstacho), ptr_context);
        //cairo_destroy(ptr_context);
    }
}


void
gtk_sensorstacho_unset_text (GtkSensorsTacho *ptr_sensorstacho)
{
    cairo_t *ptr_context = NULL;
    GdkWindow *ptr_gdkwindowsensorstacho = NULL;

    g_return_if_fail (ptr_sensorstacho != NULL);

    if (ptr_sensorstacho->text != NULL)
        g_free (ptr_sensorstacho->text);

    ptr_sensorstacho->text = NULL;
    //ptr_sensorstacho->gc = NULL;
    ptr_gdkwindowsensorstacho = gtk_widget_get_window(GTK_WIDGET(ptr_sensorstacho));

    if (ptr_gdkwindowsensorstacho != NULL)
        //ptr_context = gdk_cairo_create (ptr_gdkwindowsensorstacho);

    if (ptr_context != NULL) {
        gtk_sensorstacho_paint(GTK_WIDGET(ptr_sensorstacho), ptr_context);
        cairo_destroy(ptr_context);
    }
}


GtkWidget *
gtk_sensorstacho_new(GtkOrientation orientation, guint size)
{
    GtkSensorsTacho *ptr_sensorstacho = g_object_new(gtk_sensorstacho_get_type(), NULL);
    TRACE("enter gtk_sensorstacho_new\n");

    ptr_sensorstacho->orientation = orientation;
    ptr_sensorstacho->size = size;
    return GTK_WIDGET(ptr_sensorstacho);
}


void
gtk_sensorstacho_set_color (GtkSensorsTacho *ptr_sensorstacho, gchar *color)
{
    cairo_t *ptr_context = NULL;
    GdkWindow *ptr_gdkwindowsensorstacho = NULL;

    g_return_if_fail (ptr_sensorstacho != NULL);

    if (color == NULL) {
        gtk_sensorstacho_unset_color(ptr_sensorstacho);
        return;
    }
    else if (ptr_sensorstacho->color != NULL)
        g_free(ptr_sensorstacho->color);

    ptr_sensorstacho->color = g_strdup(color);

    ptr_gdkwindowsensorstacho = gtk_widget_get_window(GTK_WIDGET(ptr_sensorstacho));

    if (ptr_gdkwindowsensorstacho != NULL)
        //ptr_context = gdk_cairo_create (ptr_gdkwindowsensorstacho);

    if (ptr_context != NULL) {
        gtk_sensorstacho_paint(GTK_WIDGET(ptr_sensorstacho), ptr_context);
        cairo_destroy(ptr_context);
    }
}


void
gtk_sensorstacho_unset_color (GtkSensorsTacho *ptr_sensorstacho)
{
    cairo_t *ptr_context = NULL;
    GdkWindow *ptr_gdkwindowsensorstacho = NULL;

    g_return_if_fail (ptr_sensorstacho != NULL);

    if (ptr_sensorstacho->color!=NULL)
        g_free (ptr_sensorstacho->color);

    ptr_sensorstacho->color = g_strdup("#000000");

    ptr_gdkwindowsensorstacho = gtk_widget_get_window(GTK_WIDGET(ptr_sensorstacho));

    if (ptr_gdkwindowsensorstacho != NULL)
        //ptr_context = gdk_cairo_create (ptr_gdkwindowsensorstacho);

    if (ptr_context != NULL) {
        gtk_sensorstacho_paint(GTK_WIDGET(ptr_sensorstacho), ptr_context);
        cairo_destroy(ptr_context);
    }
}


static void
gtk_sensorstacho_class_init (GtkSensorsTachoClass *klass)
{
    GtkWidgetClass *widget_class;
    //GtkObjectClass *object_class;
    //GtkDrawingAreaClass *drawingarea_class;
    TRACE("enter gtk_sensorstacho_class_init\n");


    widget_class = GTK_WIDGET_CLASS (klass);
    //object_class = (GtkObjectClass *) klass;
    //drawingarea_class = (GtkDrawingAreaClass *) klass;

    //widget_class->realize = gtk_sensorstacho_realize;
    widget_class->get_request_mode = gtk_sensorstacho_get_request_mode;
    widget_class->get_preferred_width = gtk_sensorstacho_get_preferred_width;
    widget_class->get_preferred_height = gtk_sensorstacho_get_preferred_height;
    widget_class->get_preferred_width_for_height = gtk_sensorstacho_get_preferred_width_for_height;
    widget_class->get_preferred_height_for_width = gtk_sensorstacho_get_preferred_height_for_width;
    widget_class->size_allocate = gtk_sensorstacho_size_allocate;
    //widget_class->expose_event = gtk_sensorstacho_expose;
    //widget_class->button_press_event = gtk_sensorstacho_button_press;

    //drawingarea_class = GTK_DRAWING_AREA_CLASS (klass);
    widget_class->draw = gtk_sensorstacho_paint;

    //object_class->destroy = gtk_sensorstacho_destroy;

    if (font==NULL)
        font = g_strdup("Sans 12");

    TRACE("leave gtk_sensorstacho_class_init\n");
}


static void
gtk_sensorstacho_init (GtkSensorsTacho *ptr_sensorstacho)
{
    TRACE("enter gtk_sensorstacho_init\n");

    g_return_if_fail (ptr_sensorstacho != NULL);

    //ptr_sensorstacho->gc = NULL; /* can't allocate a valid GC because GtkWidget is not yet allocated */
    ptr_sensorstacho->sel = 0.0;
    ptr_sensorstacho->text = NULL;

    if (ptr_sensorstacho->color != NULL)
        g_free(ptr_sensorstacho->color);

    ptr_sensorstacho->color = g_strdup("#000000");

    //gtk_widget_set_size_request(GTK_WIDGET(ptr_sensorstacho), DEFAULT_WIDTH, DEFAULT_HEIGHT);

    TRACE("leave gtk_sensorstacho_init\n");
}


//static void
//gtk_sensorstacho_size_request (GtkWidget *widget, GtkRequisition *requisition)
//{
    ////GtkWidget *ptr_parentwidget;
    ////GtkAllocation allocation;
    //TRACE("enter gtk_sensorstacho_size_request\n");
    //g_return_if_fail(widget != NULL);
    //g_return_if_fail(GTK_IS_SENSORSTACHO(widget));
    //g_return_if_fail(requisition != NULL);

    ////DBG("size request: %d x %d. \n", requisition->width, requisition->height);

    ///* dynamic changes that scale the originally drawn picture accordingly */
    ///* set the ratio, but actually this is not needed */

    ////ptr_parentwidget = gtk_widget_get_parent (widget);

    ////gtk_widget_get_allocation (ptr_parentwidget, &allocation);
    ////DBG ("allocation for size_request: %dx%d.\n", allocation.width, allocation.height);

    ////requisition->width = MIN(allocation.width, allocation.height);
    ////GTK_WIDGET_GET_CLASS (widget)->get_preferred_width (widget, &requisition->width, &requisition->height);
    //requisition->width = DEFAULT_WIDTH;
    ////requisition->height = MIN(allocation.width, allocation.height);
    //requisition->height = DEFAULT_HEIGHT;
    //TRACE("leave gtk_sensorstacho_size_request\n");
//}

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


static void
gtk_sensorstacho_get_preferred_width(GtkWidget *widget,
                                gint      *minimal_width,
                                gint      *natural_width)
{
    //gint min_height, nat_height;
    //TRACE("enter");
    g_return_if_fail(widget != NULL);

    if (GTK_SENSORSTACHO(widget)->orientation == GTK_ORIENTATION_HORIZONTAL)
    {
        //GTK_WIDGET_GET_CLASS (widget)->get_preferred_height (widget,
                                                           //&min_height,
                                                           //&nat_height);

        //DBG("widget preferred height1: %d, %d.\n", min_height, nat_height);

    if (minimal_width)
        *minimal_width = MAX(GTK_SENSORSTACHO(widget)->size, MINIMUM_WIDTH);
    }
    else if (minimal_width)
        *minimal_width = MAX(GTK_SENSORSTACHO(widget)->size, MINIMUM_WIDTH);

    if (natural_width)
        *natural_width = GTK_SENSORSTACHO(widget)->size;

    DBG("widget preferred width2: %d, %d.\n", *minimal_width, *natural_width);

    //TRACE("leave");
}


static void
gtk_sensorstacho_get_preferred_height(GtkWidget *widget,
                                gint      *minimal_height,
                                gint      *natural_height)
{
    //gint min_width, nat_width;
    //TRACE("enter");
    g_return_if_fail(widget != NULL);

    if (GTK_SENSORSTACHO(widget)->orientation == GTK_ORIENTATION_VERTICAL)
    {
        //GTK_WIDGET_GET_CLASS (widget)->get_preferred_width (widget,
                                                           //&min_width,
                                                           //&nat_width);

        //DBG("widget preferred width1: %d, %d.\n", min_width, nat_width);

    if (minimal_height)
      *minimal_height = MAX(GTK_SENSORSTACHO(widget)->size, MINIMUM_HEIGHT);

    }
    else if (minimal_height)
      *minimal_height = MAX(GTK_SENSORSTACHO(widget)->size, MINIMUM_HEIGHT);

    if (natural_height)
      *natural_height = GTK_SENSORSTACHO(widget)->size;

    DBG("widget preferred height2: %d, %d.\n", *minimal_height, *natural_height);

    //TRACE("leave");
}


static void
gtk_sensorstacho_get_preferred_width_for_height(GtkWidget *widget,
                                gint             height,
                                gint      *minimal_width,
                                gint      *natural_width)
{
    //GtkRequisition requisition;
    g_return_if_fail(widget != NULL);

    //gtk_sensorstacho_size_request(widget, &requisition);
    *minimal_width =
    *natural_width = MAX(height, MINIMUM_WIDTH);
    DBG ("Setting preferred natural width %d for height %d.\n", *natural_width, height);
}


static void
gtk_sensorstacho_get_preferred_height_for_width(GtkWidget *widget,
                                gint      width,
                                gint      *minimal_height,
                                gint      *natural_height)
{
    //GtkRequisition requisition;
    g_return_if_fail(widget != NULL);

    //gtk_sensorstacho_size_request(widget, &requisition);
    *minimal_height =
    *natural_height = MAX(width, MINIMUM_HEIGHT);
    DBG ("Setting preferred natural height %d for width %d.\n", *natural_height, width);
}


static void
gtk_sensorstacho_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
  //cairo_t *cr;
  gint minwh;
  //GdkWindow *ptr_widgetwindow;
  //gint x, y;

  TRACE("enter gtk_sensorstacho_size_allocate\n");
  g_return_if_fail(widget != NULL);
  g_return_if_fail(GTK_IS_SENSORSTACHO(widget));
  g_return_if_fail(allocation != NULL);
  DBG ("width x height = %d x %d\n", allocation->width, allocation->height);

//ptr_widgetwindow = gtk_widget_get_window(widget);
//gdk_window_get_position(ptr_widgetwindow, &x, &y);
//DBG("window pos: %dx%d.\n", x, y);

  //DBG("minimum is %d\n", minwh);

  minwh =
  allocation->width = allocation->height = MIN(allocation->width, allocation->height);
  //DBG ("width x height = %d x %d\n", allocation->width, allocation->height);
  DBG ("x, y = %d, %d\n", allocation->x, allocation->y);
  gtk_widget_set_allocation(widget, allocation); // according to tutorials, this is the main thing of the current function

  gtk_widget_set_size_request(widget, minwh, minwh);

  if (gtk_widget_get_realized(widget))
  {
     gdk_window_move(
         gtk_widget_get_window(widget),
         allocation->x, allocation->y
         //x, y
         //allocation->width, allocation->height // determines width and height of the drawn area
     );
     ////gtk_window_resize(gtk_widget_get_parent_window(widget), minwh, minwh);
  }
  else
  {
    //DBG ("width x height = %d x %d\n", allocation->width, allocation->height);
  }
  //cr = gdk_cairo_create (gtk_widget_get_window(widget));
  //gtk_sensorstacho_paint(widget, cr);

  TRACE("leave gtk_sensorstacho_size_allocate\n");
}


//static void
//gtk_sensorstacho_realize (GtkWidget *widget)
//{
  //GdkWindowAttr attributes;
  //guint attributes_mask;
  //int minwh;
  //GtkAllocation allocation;
  ////cairo_t *cr;
  ////GdkRGBA color;
  ////GtkStyleContext *context;

  ////context = gtk_widget_get_style_context (widget);
  //TRACE("enter gtk_sensorstacho_realize\n");

  //g_return_if_fail(widget != NULL);
  //g_return_if_fail(GTK_IS_CPU(widget));

  ////GTK_WIDGET_SET_FLAGS(widget, GTK_REALIZED);

  //attributes.window_type = GDK_WINDOW_CHILD;
  //gtk_widget_get_allocation(widget, &allocation);
  //attributes.x = allocation.x;
  //attributes.y = allocation.y;

  ///* define the minimum size; otherwise the area is not painted in the beginning */
  ///* need square drawable area */
  //minwh = min (gtk_widget_get_allocated_width(widget), gtk_widget_get_allocated_height(widget));
  //attributes.width = minwh; // DEFAULT_WIDTH;
  //attributes.height = minwh; //DEFAULT_HEIGHT;

  //attributes.wclass = GDK_INPUT_OUTPUT;
  //attributes.event_mask = gtk_widget_get_events(widget) | GDK_EXPOSURE_MASK;

  //attributes_mask = GDK_WA_X | GDK_WA_Y;

  //gtk_widget_set_window(widget, gdk_window_new(
     //gtk_widget_get_parent_window (widget),
     //& attributes, attributes_mask)
  //);

  //gtk_widget_set_realized(widget, TRUE);

  //gdk_window_set_user_data(gtk_widget_get_window(widget), widget);

  ////cr = gdk_cairo_create (gtk_widget_get_window(widget));
  ////gtk_render_background (context, cr, 0, 0, minwh, minwh);


  ////cairo_arc (cr,
             ////minwh / 2.0, minwh / 2.0,
             ////minwh / 2.0,
             ////0, 2 * G_PI);


  ////gtk_style_context_get_color (context,
                               ////gtk_style_context_get_state (context),
                               ////&color);
  ////color.red = 1;
  ////gdk_cairo_set_source_rgba (cr, &color);

  ////cairo_fill (cr);

  ////widget->style = gtk_style_attach(widget->style, gtk_widget_get_window(widget));
  ////gtk_style_set_background(widget->style, gtk_widget_get_window(widget), GTK_STATE_NORMAL);
  ////gtk_widget_override_background_color(widget, 0, ?);

  ////ptr_context = gdk_cairo_create (gtk_widget_get_window(GTK_WIDGET(cpu)));
  ////gtk_sensorstacho_paint(widget, cr);
  //TRACE("leave gtk_sensorstacho_realize\n");
//}


//static gboolean
//gtk_sensorstacho_expose(GtkWidget *widget, GdkEventExpose *event)
//{
  //TRACE("enter gtk_sensorstacho_expose\n");
  //g_return_val_if_fail(widget != NULL, FALSE);
  //g_return_val_if_fail(GTK_IS_CPU(widget), FALSE);
  //g_return_val_if_fail(event != NULL, FALSE);
  //DBG("event: %d\n", event->type);

  //gtk_sensorstacho_paint(widget);

  //TRACE("leave gtk_sensorstacho_expose\n");
  //return TRUE;
//}


gboolean
gtk_sensorstacho_paint (GtkWidget *widget,
                 cairo_t *ptr_cairo_context/*, gpointer data*/)
{
    gchar *text;
    GtkStyleContext *context;
    GdkRGBA color;
    int i;
    double percent;
    PangoFontDescription *desc;
    gint width, height;
    gint xc, yc;
    double degrees_135 = (135) * G_PI / 180;
    double degrees_45minusI;
    GtkAllocation allocation;

    TRACE("enter gtk_sensorstacho_paint\n");

    if (ptr_cairo_context==NULL)
    {
      DBG("Cairo drawing context is NULL.\n");
      return FALSE;
    }

    //DBG ("Widget=0x%llX, Window=0x%llX\n", (unsigned long long int) widget, (unsigned long long int) gtk_widget_get_parent_window(widget));

    context = gtk_widget_get_style_context (widget);

    if (context==NULL)
    {
        // TODO FIXME: The following might be responsible for the widget not working in newly allocated sensors plugin
        DBG("widget context is NULL.\n");
        return FALSE;
    }

    gtk_widget_get_allocation(widget, &allocation);
    //allocation.width = allocation.height = MIN(allocation.width, allocation.height);
    //gtk_widget_size_allocate(widget, &allocation);

    width = gtk_widget_get_allocated_width (widget);
    height = gtk_widget_get_allocated_height (widget);
    DBG ("allocated width x height = %d x %d\n", width, height);

    width = height = MIN(width, height);
    //gtk_widget_set_size_request(widget, width, height);

    gtk_style_context_get_color (context,
                                 gtk_style_context_get_state (context),
                                 &color);

    percent = GTK_SENSORSTACHO(widget)->sel;
    if (percent>1.0)
        percent = 1.0;

    xc =
    //allocation.x // /2
    //+
    width/2
     //+3
    ;
    yc =
    //allocation.y // /2
    //+
    height/2
     //+ 7
    ;

    DBG ("using width x height = %d x %d\n", width, height);
    DBG("tacho: x,y = %d, %d\n", allocation.x, allocation.y);



    /* initialize color values appropriately */
    color.red = 1.0;
    color.green = 1.0;
    color.blue = 0.25;

    if (percent < 0.5)
        color.red = 2*percent;

    if (percent > 0.5)
        color.green = 2.0 - 2*percent;

    /* draw circular gradient */
    for (i=(1-percent)*THREE_QUARTER_CIRCLE; i<THREE_QUARTER_CIRCLE; i++)
    {
        //DBG ("%d: %f => %f,%f,%f\n", i, percent, color.red, color.green, color.blue);

        gdk_cairo_set_source_rgba (ptr_cairo_context, &color);

        degrees_45minusI = (45-i) * G_PI / 180;

        cairo_arc (ptr_cairo_context,
           xc, yc,
           width/2-2,
           degrees_135,
           degrees_45minusI);

        cairo_line_to (ptr_cairo_context, xc, yc);

        cairo_arc (ptr_cairo_context, xc, yc, width/2-4, degrees_45minusI, degrees_45minusI);
        cairo_line_to (ptr_cairo_context, xc, yc);

        cairo_fill (ptr_cairo_context);

        if (i>(0.5*THREE_QUARTER_CIRCLE - 1))
            color.red -= 2*COLOR_STEP;

        if (i<(0.5*THREE_QUARTER_CIRCLE - 1))
            color.green += 2*COLOR_STEP;
    }

    /* white right part */
    cairo_arc (ptr_cairo_context,
               xc, yc,
               width/2-2,
               degrees_135, 45 * G_PI / 180);

    cairo_line_to (ptr_cairo_context, xc, yc);

    cairo_arc (ptr_cairo_context, xc, yc, width/2-2, degrees_135, degrees_135);
    cairo_line_to (ptr_cairo_context, xc, yc);

    /* black border */
    cairo_set_line_width (ptr_cairo_context, 0.5);

    color.red = 0.0;
    color.green = 0.0;
    color.blue = 0.0;
    gdk_cairo_set_source_rgba (ptr_cairo_context, &color);

    cairo_stroke (ptr_cairo_context);

    if (GTK_SENSORSTACHO(widget)->text != NULL) {
        PangoContext *ptr_style_context = gtk_widget_get_pango_context (widget); // pango_context_new ();
        PangoLayout *layout = pango_layout_new (ptr_style_context);

        pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);
        pango_layout_set_width (layout, width);

        text = g_strdup_printf("<span color=\"%s\">%s</span>", GTK_SENSORSTACHO(widget)->color, GTK_SENSORSTACHO(widget)->text);
        DBG("pango layout markup text: %s.\n", text);
        pango_layout_set_markup (layout, text, -1); // with null-termination, no need to give length explicitly

        desc = pango_font_description_from_string(font);
        pango_layout_set_font_description (layout, desc);
        pango_font_description_free (desc);

        pango_cairo_update_layout (ptr_cairo_context, layout);

        pango_layout_get_size (layout, &width, &height);

        DBG("width, height, PANGO_SCALE: %d, %d, %d", width, height, PANGO_SCALE);

        cairo_move_to (ptr_cairo_context, xc, yc - height / 2 / PANGO_SCALE);
        pango_cairo_show_layout (ptr_cairo_context, layout);
    }

    TRACE("leave gtk_sensorstacho_paint\n");
    return TRUE;
}


//static void
//gtk_sensorstacho_destroy (GtkObject *object)
//{
  //GtkSensorsTacho *cpu;
  //GtkSensorsTachoClass *klass;
  //TRACE("enter gtk_sensorstacho_destroy\n");

  //g_return_if_fail(object != NULL);
  //g_return_if_fail(GTK_IS_CPU(object));

  //cpu = gtk_sensorstacho(object);

  ///* gdk_gc_destroy(cpu->gc); */

  //if (cpu->text!=NULL)
  //{
    //g_free (cpu->text);
    //cpu->text = NULL;
  //}

  //if (cpu->color!=NULL)
  //{
    //g_free (cpu->color);
    //cpu->color = NULL;
  //}

  //klass = gtk_type_class(gtk_widget_get_type());

  //if (GTK_OBJECT_CLASS(klass)->destroy) {
     //(* GTK_OBJECT_CLASS(klass)->destroy) (object);
  //}
  //TRACE("leave gtk_sensorstacho_destroy\n");
//}


void
gtk_sensorstacho_set_value (GtkSensorsTacho *ptr_sensorstacho, gdouble value)
{
  cairo_t *ptr_context = NULL;
  GdkWindow *ptr_gdkwindowsensorstacho = NULL;
  TRACE("enter gtk_sensorstacho_set_value\n");

  //GdkRegion *region;
  if (value<0)
    value = 0.0;
  else if (value>1.0)
    value = 1.0;

  //region = gdk_drawable_get_clip_region (GTK_WIDGET(cpu)->window);
  //gdk_window_invalidate_region (GTK_WIDGET(cpu)->window, region, TRUE);
  //gdk_window_process_updates (GTK_WIDGET(cpu)->window, TRUE);
  //gtk_sensorstacho_paint(GTK_WIDGET(cpu));
  //gboolean result = FALSE;
  //g_signal_emit_by_name(GTK_WIDGET(cpu), "draw", &result);

  if (ptr_sensorstacho != NULL) {
      ptr_sensorstacho->sel = value;
      ptr_gdkwindowsensorstacho = gtk_widget_get_window(GTK_WIDGET(ptr_sensorstacho));
    }

  if (ptr_gdkwindowsensorstacho != NULL)
      //ptr_context = gdk_cairo_create (ptr_gdkwindowsensorstacho);

  if (ptr_context != NULL) {
      gtk_sensorstacho_paint(GTK_WIDGET(ptr_sensorstacho), ptr_context);
      cairo_destroy(ptr_context);
  }

  TRACE("leave gtk_sensorstacho_set_value\n");
}


//static gboolean
//gtk_sensorstacho_button_press (GtkWidget      *widget,
                       //GdkEventButton *event)
//{
  //DBG("obtained mouse event.\n");
  //// gtk_sensorstacho(widget)->parent_class -> send event
  //return FALSE; // propagate event further
//}


void gtk_sensorstacho_set_size(GtkSensorsTacho *tacho, guint size)
{
  if (tacho)
    tacho->size = size;
}
