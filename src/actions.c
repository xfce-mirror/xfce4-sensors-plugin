/* $Id$ */

/*  Copyright 2008-2010 Fabian Nowak (timystery@arcor.de)
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

/* Note for programmers and editors: Try to use 4 spaces instead of Tab! */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/* Gtk includes */
#include <glib/gprintf.h>
#include <gtk/gtk.h>

/* Xfce includes */
//#include <libxfcegui4/libxfcegui4.h>
#include <libxfce4ui/libxfce4ui.h>

/* Package includes */
#include <middlelayer.h>
#include <sensors-interface.h>

/* Local includes */
#include "actions.h"
#include "interface.h"

/* foward declaration */
gboolean
refresh_sensor_data (t_sensors_dialog *sd);

/* actual implementations */
gboolean
refresh_sensor_data (t_sensors_dialog *sd)
{
  t_sensors *sensors;
    int i, index_feature, res;
    double sensorFeature;
    gchar *tmp;
    t_chipfeature *chipfeature;
    t_chip *chip;
    
    TRACE ("enters refresh_sensor_data");

    g_return_val_if_fail (sd != NULL, FALSE);

    sensors = sd->sensors;

    for (i=0; i < sensors->num_sensorchips; i++) {
        chip = (t_chip *) g_ptr_array_index (sensors->chips, i);
        g_assert (chip!=NULL);

        for (index_feature = 0; index_feature<chip->num_features; index_feature++) {
            chipfeature = g_ptr_array_index (chip->chip_features, index_feature);
            g_assert (chipfeature!=NULL);

            if ( chipfeature->valid == TRUE)
            {
                res = sensor_get_value (chip, chipfeature->address,
                                                    &sensorFeature,
                                                    &(sensors->suppressmessage));

                if ( res!=0 ) {
                    /* FIXME: either print nothing, or undertake appropriate action,
                     * or pop up a message box. */
                    g_printf ( _("Sensors Viewer:\n"
                    "Seems like there was a problem reading a sensor feature "
                    "value.\nProper proceeding cannot be guaranteed.\n") );
                    break;
                }
                tmp = g_new (gchar, 0);
                format_sensor_value (sensors->scale, chipfeature,
                                     sensorFeature, &tmp);

                g_free (chipfeature->formatted_value);
                chipfeature->formatted_value = g_strdup (tmp);
                chipfeature->raw_value = sensorFeature;

                g_free (tmp);
            } /* end if chipfeature->valid */
        }
    }

    TRACE ("leaves refresh_sensor_data");

    return TRUE;
}

//void
//refresh_tree_view (t_sensors_dialog *sd)
//{
  //GtkTreeModel *  model;
  
  //model = gtk_tree_view_get_model(GTK_TREE_VIEW(sd->myTreeView));
  
//}

void gtk_widget_unparent_ext (GtkWidget *widget, gpointer data);

void
gtk_widget_unparent_ext (GtkWidget *widget, gpointer data)
{
  gtk_widget_unparent(widget);
}

void
refresh_tacho_view (t_sensors_dialog *sd)
{
  //GtkTreeModel *  model;
  int i, index_feature, row=0, col=0;
  t_sensors *sensors;
  t_chipfeature *chipfeature;
  t_chip *chip;
  GtkWidget *table; //, *widget;
  //GList *list;
  gdouble d;
  gchar *myToolTipText;
  
  TRACE ("enters refresh_tacho_view");
  
  g_return_if_fail (sd!=NULL);
  
  sensors = sd->sensors;
  
  //model = gtk_tree_view_get_model(GTK_TREE_VIEW(sd->myTreeView));
  
  table = sensors->widget_sensors;
  
  //list = gtk_container_get_children (GTK_CONTAINER(table));
  //if (list!=NULL)
  //{
    //for (widget=(GtkWidget *) (list->data); widget!=NULL; list=list->next)
    //{
      //gtk_widget_unparent(widget);
    //}
  //}
  //gtk_container_foreach(GTK_CONTAINER(table), (GtkCallback) gtk_widget_hide, NULL);
  //gtk_widget_destroy(table);
  //table = sensors->widget_sensors = gtk_table_new(5, 4, TRUE);
  //gtk_widget_show(table);
  
    for (i=0; i < sensors->num_sensorchips; i++) 
    {
        chip = (t_chip *) g_ptr_array_index (sensors->chips, i);
        g_assert (chip!=NULL);

        for (index_feature = 0; index_feature<chip->num_features; index_feature++)
        {
            chipfeature = g_ptr_array_index (chip->chip_features, index_feature);
            g_assert (chipfeature!=NULL);

            if ( chipfeature->valid == TRUE && chipfeature->show == TRUE)
            {
                // actually, the idea is to move the tacho widgets in the table; but Gtk fails at this point: 
                //  there is neither a gtk_table_move nor does it allow to unparent a widget and attach it at a new positon,
                //  so the container is removed and destroyed for safety.
                if ((sensors->tachos [i][index_feature]!=NULL) && (sensors->tachos [i][index_feature]->parent!=NULL))
                {
                    //gtk_widget_unparent (sensors->tachos [i][index_feature]);
                    gtk_container_remove(GTK_CONTAINER(table), sensors->tachos [i][index_feature]);
                    //gtk_widget_destroy(sensors->tachos [i][index_feature]);
                }
                //else if (sensors->tachos [i][index_feature]==NULL )
                //{
                  // sensors->tachos[i][index_feature] is now destroyed due to bad GTK function naming and implementation of gtk_container_remove; gtk_widget_unparent does not work at all from within here.
                    sensors->tachos[i][index_feature] = gtk_cpu_new();
                //}
              
                //gtk_cpu_set_value (GTK_CPU(sensors->tachos [i][index_feature]), (chipfeature->raw_value - chipfeature->min_value) / ( chipfeature->max_value - chipfeature->min_value));
                d = (chipfeature->raw_value - chipfeature->min_value) / ( chipfeature->max_value - chipfeature->min_value);
                if (d<0.0) 
                    d=0.0;
                else if (d>1.0) 
                    d=1.0;
                    
                GTK_CPU(sensors->tachos [i][index_feature])->sel = d;
                //gtk_cpu_set_text (GTK_CPU(sensors->tachos [i][index_feature]), chipfeature->name);
                GTK_CPU(sensors->tachos [i][index_feature])->text = g_strdup(chipfeature->name);
                GTK_CPU(sensors->tachos [i][index_feature])->color = g_strdup(chipfeature->color);
                //gtk_cpu_expose(sensors->tachos [i][index_feature], NULL);
                
                
                gtk_widget_set_size_request(sensors->tachos [i][index_feature], 64, 64);
                
                gtk_table_attach_defaults(GTK_TABLE(table), sensors->tachos [i][index_feature], col, col+1, row, row+1);
                gtk_widget_show (sensors->tachos [i][index_feature]);
                //gtk_cpu_paint(sensors->tachos [i][index_feature]);
                //gtk_cpu_expose(sensors->tachos [i][index_feature], NULL);
                
    /* #if GTK_VERSION < 2.11 */
    myToolTipText = g_strdup(_("You can change a feature's properties such as name, colours, min/max value by double-clicking the entry, editing the content, and pressing \"Return\" or selecting a different field."));
    gtk_tooltips_set_tip (tooltips, GTK_WIDGET(sensors->tachos [i][index_feature]),
                          myToolTipText, NULL);
                
                if (col>=3) {
                    row++;
                    col = 0;
                }
                else
                    col++;
                
                if (row>=5)
                    return; /* or resize the table after reading the property n-rows of GtkTableClass*/
            }
        }
    }
    TRACE ("leaves refresh_tacho_view");
}

gboolean
refresh_view (gpointer data)
{
  t_sensors_dialog *sd;
  
  TRACE ("enters refresh_view");
  
  g_return_val_if_fail (data != NULL, FALSE);
  sd = (t_sensors_dialog *) data;
  
  if (!refresh_sensor_data (sd))
    return FALSE;
  
  reload_listbox(sd);
  
  refresh_tacho_view (sd);
  
  /* refresh all the tacho elements in the notebook/table view */
  
  
  TRACE ("leaves refresh_view");
  return TRUE;
}
