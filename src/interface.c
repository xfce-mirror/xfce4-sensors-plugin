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

/* Xfce includes */
#include <libxfcegui4/libxfcegui4.h>

/* Package includes */
#include <cpu.h>
#include <sensors-interface.h>
#include <sensors-interface-common.h>

/* Local includes */
#include "callbacks.h"
#include "interface.h"

/* forward declarations */
void add_tachos_box (GtkWidget *child, t_sensors_dialog *);
void add_notebook (GtkWidget *box, t_sensors_dialog *sd);


/* actual implementations */

void
add_tachos_box (GtkWidget *child, t_sensors_dialog *sd)
{
    GtkWidget *table;
    
    table = gtk_table_new (5, 4, TRUE);
    gtk_table_set_row_spacings(GTK_TABLE(table), BORDER);
    gtk_table_set_col_spacings(GTK_TABLE(table), BORDER);
    gtk_widget_show (table); 
    sd->sensors->widget_sensors = table;
    gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW(child), table);
}


void
add_notebook (GtkWidget *box, t_sensors_dialog *sd)
{
    GtkWidget *nb, *child, *tab_label, *scr_win, *fontbutton;
    int res;
    
    nb = gtk_notebook_new();
    gtk_widget_show (nb);
    
    child = gtk_vbox_new (FALSE, BORDER);
    gtk_container_set_border_width (GTK_CONTAINER (child), BORDER);
    gtk_widget_show (child); 
    
    add_type_box(child, sd);
    add_sensor_settings_box(child, sd);
    add_temperature_unit_box(child, sd);
    add_update_time_box (child, sd);
    
    tab_label = gtk_label_new_with_mnemonic(_("_Overview"));
    gtk_widget_show (tab_label); 
    res = gtk_notebook_append_page       (GTK_NOTEBOOK(nb), child, tab_label);
    
    child = gtk_vbox_new (FALSE, BORDER);
    gtk_container_set_border_width (GTK_CONTAINER (child), BORDER);
    gtk_widget_show (child); 
    
    scr_win  = gtk_scrolled_window_new( NULL,  NULL);
    gtk_box_pack_start(GTK_BOX(child), scr_win, TRUE, TRUE, 0);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scr_win), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_show_all (scr_win); 
    
    add_tachos_box (scr_win, sd);
    
    fontbutton = gtk_font_button_new();
    g_signal_connect (G_OBJECT(fontbutton), "font-set", G_CALLBACK(on_font_set), NULL);
    gtk_widget_show (fontbutton); 
    gtk_box_pack_end (GTK_BOX(child), fontbutton, FALSE, FALSE, 0);
    
    tab_label = gtk_label_new_with_mnemonic(_("_Tachometers"));
    gtk_widget_show (tab_label); 
    res = gtk_notebook_append_page       (GTK_NOTEBOOK(nb), child, tab_label);
    
    gtk_box_pack_start(GTK_BOX(box), nb, TRUE, TRUE, 0);
}


GtkWidget *
create_main_window (t_sensors_dialog *sd)
{

    GtkWidget *dlg, *vbox;

    /* start and populate */
    dlg = xfce_titled_dialog_new_with_buttons(
                _("Sensors Viewer"),
                NULL,
                GTK_DIALOG_NO_SEPARATOR,
                GTK_STOCK_CLOSE,
                GTK_RESPONSE_OK,
                NULL
            );
    gtk_widget_show (dlg);
    
    gtk_window_set_icon_name(GTK_WINDOW(dlg),"xfce-sensors");

    gtk_container_set_border_width (GTK_CONTAINER (dlg), BORDER>>1);

    vbox = GTK_DIALOG (dlg)->vbox;
    gtk_box_set_spacing(GTK_BOX(vbox), BORDER>>1);
    
    gtk_widget_show (vbox);

    sd->dialog = dlg;

    sd->myComboBox = gtk_combo_box_new_text();
    gtk_widget_show (sd->myComboBox);

    init_widgets (sd);

    gtk_combo_box_set_active (GTK_COMBO_BOX(sd->myComboBox), 0);

    
    add_notebook (vbox, sd);
        
    gtk_window_set_default_size (GTK_WINDOW(dlg), sd->sensors->preferred_width, sd->sensors->preferred_height);

    g_signal_connect (G_OBJECT(dlg), "response", G_CALLBACK(on_main_window_response), sd); // also captures the dialog-destroy event and the closekeybinding-pressed event
    //g_signal_connect (G_OBJECT(dlg), "destroy", G_CALLBACK(on_main_window_response), sd);

    return dlg;
}
