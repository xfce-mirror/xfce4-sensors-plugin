/* $Id$ */

/*  Copyright 2008-2016 Fabian Nowak (timystery@arcor.de)
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
refresh_sensor_data (t_sensors_dialog *ptr_sensors_dialog_structure)
{
  t_sensors *sensors;
    int idx_chip, idx_feature, result;
    double val_sensor_feature;
    gchar *tmp;
    t_chipfeature *chipfeature;
    t_chip *ptr_chip_structure;
    
    TRACE ("enters refresh_sensor_data");

    g_return_val_if_fail (ptr_sensors_dialog_structure != NULL, FALSE);

    sensors = ptr_sensors_dialog_structure->sensors;

    for (idx_chip=0; idx_chip < sensors->num_sensorchips; idx_chip++) {
        ptr_chip_structure = (t_chip *) g_ptr_array_index (sensors->chips, idx_chip);
        g_assert (ptr_chip_structure!=NULL);

        for (idx_feature = 0; idx_feature<ptr_chip_structure->num_features; idx_feature++) {
            chipfeature = g_ptr_array_index (ptr_chip_structure->chip_features, idx_feature);
            g_assert (chipfeature!=NULL);

            if ( chipfeature->valid == TRUE)
            {
                result = sensor_get_value (ptr_chip_structure, chipfeature->address,
                                                    &val_sensor_feature,
                                                    &(sensors->suppressmessage));

                if ( result!=0 ) {
                    /* FIXME: either print nothing, or undertake appropriate action,
                     * or pop up a message box. */
                    g_printf ( _("Sensors Viewer:\n"
                    "Seems like there was a problem reading a sensor feature "
                    "value.\nProper proceeding cannot be guaranteed.\n") );
                    break;
                }
                tmp = g_new (gchar, 0);
                format_sensor_value (sensors->scale, chipfeature,
                                     val_sensor_feature, &tmp);

                g_free (chipfeature->formatted_value);
                chipfeature->formatted_value = g_strdup (tmp);
                chipfeature->raw_value = val_sensor_feature;

                g_free (tmp);
            } /* end if chipfeature->valid */
        }
    }

    TRACE ("leaves refresh_sensor_data");

    return TRUE;
}


void
refresh_tacho_view (t_sensors_dialog *ptr_sensors_dialog_structure)
{
    int idx_chip, idx_feature, row_tacho_table=0, col_tacho_table=0;
    t_sensors *ptr_sensors_structure;
    t_chipfeature *ptr_chipfeature_structure;
    t_chip *ptr_chip_structure;
    GtkWidget *ptr_wdgt_table;
    gdouble val_fill_degree;

    TRACE ("enters refresh_tacho_view");

    g_return_if_fail (ptr_sensors_dialog_structure!=NULL);

    ptr_sensors_structure = ptr_sensors_dialog_structure->sensors;

    ptr_wdgt_table = ptr_sensors_structure->widget_sensors;
  
    for (idx_chip=0; idx_chip < ptr_sensors_structure->num_sensorchips; idx_chip++) 
    {
        ptr_chip_structure = (t_chip *) g_ptr_array_index (ptr_sensors_structure->chips, idx_chip);
        g_assert (ptr_chip_structure!=NULL);

        for (idx_feature = 0; idx_feature<ptr_chip_structure->num_features; idx_feature++)
        {
            ptr_chipfeature_structure = g_ptr_array_index (ptr_chip_structure->chip_features, idx_feature);
            g_assert (ptr_chipfeature_structure!=NULL);

            if ( ptr_chipfeature_structure->valid == TRUE && ptr_chipfeature_structure->show == TRUE)
            {
                // actually, the idea is to move the tacho widgets in the table; but Gtk fails at this point: 
                //  there is neither a gtk_table_move nor does it allow to unparent a widget and attach it at a new positon,
                //  so the container is removed and destroyed for safety.
                if ((ptr_sensors_structure->tachos [idx_chip][idx_feature]!=NULL) && (ptr_sensors_structure->tachos [idx_chip][idx_feature]->parent!=NULL))
                {
                    gtk_container_remove(GTK_CONTAINER(ptr_wdgt_table), ptr_sensors_structure->tachos [idx_chip][idx_feature]);
                }
                ptr_sensors_structure->tachos[idx_chip][idx_feature] = gtk_cpu_new();
                    
                val_fill_degree = (ptr_chipfeature_structure->raw_value - ptr_chipfeature_structure->min_value) / ( ptr_chipfeature_structure->max_value - ptr_chipfeature_structure->min_value);
                if (val_fill_degree<0.0) 
                    val_fill_degree=0.0;
                else if (val_fill_degree>1.0) 
                    val_fill_degree=1.0;
                    
                GTK_CPU(ptr_sensors_structure->tachos [idx_chip][idx_feature])->sel = val_fill_degree;
                char str_widget_tooltip_text[128];
                snprintf(str_widget_tooltip_text, 128, "<b>%s</b>\n%s: %s", ptr_chip_structure->sensorId, ptr_chipfeature_structure->name, ptr_chipfeature_structure->formatted_value);
                gtk_widget_set_tooltip_markup (GTK_WIDGET(ptr_sensors_structure->tachos [idx_chip][idx_feature]), str_widget_tooltip_text);
                GTK_CPU(ptr_sensors_structure->tachos [idx_chip][idx_feature])->text = g_strdup(ptr_chipfeature_structure->name);
                GTK_CPU(ptr_sensors_structure->tachos [idx_chip][idx_feature])->color = g_strdup(ptr_chipfeature_structure->color);
                
                
                gtk_widget_set_size_request(ptr_sensors_structure->tachos [idx_chip][idx_feature], 64, 64);
                
                gtk_table_attach_defaults(GTK_TABLE(ptr_wdgt_table), ptr_sensors_structure->tachos [idx_chip][idx_feature], col_tacho_table, col_tacho_table+1, row_tacho_table, row_tacho_table+1);
                gtk_widget_show (ptr_sensors_structure->tachos [idx_chip][idx_feature]);
                                
                if (col_tacho_table>=3) {
                    row_tacho_table++;
                    col_tacho_table = 0;
                }
                else
                    col_tacho_table++;
                
                if (row_tacho_table>=5)
                    return; /* or resize the table after reading the property n-rows of GtkTableClass*/
            }
        }
    }
    TRACE ("leaves refresh_tacho_view");
}


gboolean
refresh_view (gpointer data)
{
    t_sensors_dialog *ptr_sensors_dialog;
    gboolean return_value = FALSE;
    
    TRACE ("enters refresh_view");
    
    g_return_val_if_fail (data != NULL, FALSE);
    ptr_sensors_dialog = (t_sensors_dialog *) data;
    
    if (refresh_sensor_data (ptr_sensors_dialog))
    {
        reload_listbox(ptr_sensors_dialog);
        
        refresh_tacho_view (ptr_sensors_dialog); /* refresh all the tacho elements in the notebook/table view */
        
        return_value = TRUE;
    }
    TRACE ("leaves refresh_view");
    return return_value;
}
