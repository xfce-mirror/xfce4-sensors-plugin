/*  Copyright 2004 Fabian Nowak (timystery@arcor.de)
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

/* This plugin requires libsensors-1 and its headers !*/

#include <sensors/sensors.h>

#include <libxfce4util/i18n.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/xfce_clock.h>

#include <panel/global.h>
#include <panel/controls.h>
#include <panel/icons.h>
#include <panel/plugins.h>

#include <glib/gprintf.h>

#define BORDER 6

/*  Sensors module
 *  ------------
 */
typedef struct {
    /* eventbox to catch events */
    GtkWidget *eventbox;
    
    /* our XfceSensors widget */
    GtkWidget *sensors;
    
    /* panel value display */
    GtkWidget *panelValuesLabel;

    /* update the tooltip */
    gint timeout_id, timeout_id2;

    /* font size for display in panel */
    gchar* fontSize;
    gint fontSizeNumerical;
    
    /* panel size to compute number of cols/columns */
    gint panelSize;
    
    /* show title in panel */
    gboolean showTitle;
    
    /* sensor update time */
    gint sensorUpdateTime;
                
    /* sensor relevant stuff */
    /* no problem if less than 11 sensors, else will have to enlarge the 
        following arrays. NYI!! */
    gint sensorNumber;
    gint sensorsCount[10];
    
    /* contains structure from libsensors */
    const sensors_chip_name *chipName[10];
    
    /* formatted sensor chip names, e.g. 'asb-100-45' */
    gchar *sensorId[10];
    
    /* unformatted sensor feature names, e.g. 'Vendor' */
    gchar *sensorNames[10][256];
    
    /* formatted (%f5.2) sensor feature values */
    gchar *sensorValues[10][256];

    /* TRUE if sensorNames are set */
    gboolean sensorValid[10][256];
    
    /* show sensor in panel */
    gboolean sensorCheckBoxes[10][256];
    
    /* sensor colors in panel */
    gchar *sensorColors[10][256];
    
    /* number in list <--> number in array */
    gint sensorAddress[10][256];    
}
t_sensors;


/* sensor panel widget
 * -------------------
 */
typedef struct {
    /* the sensors structure */
    t_sensors *sensors;

    /* controls dialog */
    GtkWidget *dialog;

    /* sensors options */
    GtkWidget *type_menu;

    /* Gtk stuff */
    GtkWidget *myComboBox;
    GtkWidget *myFrame;
    GtkWidget *mySensorLabel;
    GtkWidget *myTreeView;
    GtkTreeStore *myListStore[10];    
}
SensorsDialog;


/* draw label with sensor values into panel's vbox */
gboolean
sensors_show_panel (gpointer data) 
{
/* REMARK:
    Since sensors_show_panel is called with the same period as
    update_tooltip and update_tooltip already reads in new
    sensor values, this isn't done again in here */

    /* g_printf(" sensors_show_panel\n"); */

    g_return_val_if_fail (data != NULL, FALSE);

    t_sensors *st = (t_sensors *) data;
    
    /* add labels */
    gint chipNumber = 0;
    gint itemsToDisplay=0;
    
    /* gchar *myLabelText = (gchar *) g_malloc(1024); */ 
    /* unfortunately doesn't work,
       so I duplicate the string everywhere
       to avoid missing the first label lines.
       FIX ME! */
    
    gchar *myLabelText;
    
    if (st->showTitle == TRUE) {
        /* g_snprintf(myLabelText, 1024, 
          "<span foreground=\"#000000\" size=\"%s\"><b>Sensors</b></span> \n",
          st->fontSize); */
        myLabelText = g_strdup_printf(_(
          "<span foreground=\"#000000\" size=\"%s\"><b>Sensors</b></span> \n"), 
          st->fontSize);
    }
    else /* nul-terminate the string for further concatenating */
        /* g_sprintf(myLabelText, ""); */ 
        myLabelText = g_strdup("");
    
    /* count number of checked sensors to display.
       this could also be done by every toggle/untoggle action
       by putting this variable into t_sensors */
    while (chipNumber < st->sensorNumber) {
    
        gint chipFeature = 0;
    
        while (chipFeature < 256) {
        
            if (st->sensorCheckBoxes[chipNumber][chipFeature] == TRUE)
                itemsToDisplay++;

            chipFeature ++;
        }

        chipNumber++;
    }
    
    gint numRows, numCols;
    
    /* g_printf("  .. panel size: %i \n", st->panelSize);
    g_printf("  .. items to display: %i \n" , itemsToDisplay); */
    
    switch (st->panelSize) {
        case 0: if (st->fontSizeNumerical==0) numRows = 2;
                else if (st->fontSizeNumerical==4) numRows = 0;
                else numRows = 1;
                break;
        case 1: if (st->fontSizeNumerical==0) numRows = 3;
                 else if (st->fontSizeNumerical==1 
                       || st->fontSizeNumerical==2) numRows = 2;
                else numRows = 1;
                break;
        case 2: if (st->fontSizeNumerical==0) numRows = 4;
                else if (st->fontSizeNumerical==1 
                      || st->fontSizeNumerical==2) numRows = 3;
                else numRows = 2;
                break;
        default: if (st->fontSizeNumerical==0) numRows = 5;
                 else if (st->fontSizeNumerical==1 
                       || st->fontSizeNumerical==2) numRows = 4;
                 else numRows = 3;
    }
    
    
    if (st->showTitle == FALSE ) {
        numRows++; /* we can draw one more item per column */
        
        /* draw the title if no item is to be displayed */
        /* This allows a user to still find the plugin */
        /* might also be replaced by approximately two spaces */
        if (itemsToDisplay==0) 
            /* g_sprintf(myLabelText, 1024,
              "<span foreground=\"#000000\" size=\"%s\"><b>Sensors</b></span>",
              st->fontSize); */
            myLabelText = g_strdup_printf(_(
              "<span foreground=\"#000000\" size=\"%s\"><b>Sensors</b></span>"),
              st->fontSize);
    }
    
    if ( numRows > 1) {
        if( itemsToDisplay > numRows )
            /* the following is a simple integer ceiling function */
            numCols = (itemsToDisplay%numRows == 0)? 
                    itemsToDisplay/numRows : itemsToDisplay/numRows+1;
        else
            numCols = 1;
        }
    else 
        numCols = itemsToDisplay;
    
    gint currentColumn = 0;
    
    chipNumber=0;
    while (chipNumber < st->sensorNumber) {
    
        gint chipFeature = 0;
    
        while (chipFeature < 256) {
        
            if (st->sensorCheckBoxes[chipNumber][chipFeature] == TRUE) {
                   
                /* g_snprintf(myLabelText, 1024, 
                    "%s<span foreground=\"%s\" size=\"%s\">%s</span>", 
                    myLabelText, st->sensorColors[chipNumber][chipFeature], 
                    st->fontSize, st->sensorValues[chipNumber][chipFeature]);*/
                myLabelText = g_strconcat (myLabelText, "<span foreground=\"", 
                    st->sensorColors[chipNumber][chipFeature], "\" size=\"", 
                    st->fontSize, "\">", 
                    st->sensorValues[chipNumber][chipFeature], "</span>", 
                    NULL);
            
                if (currentColumn < numCols-1) {
                    /* g_snprintf(myLabelText, 1024, "%s \t", myLabelText); */
                    myLabelText = g_strconcat (myLabelText, " \t", NULL);
                    currentColumn++;
                }
                else if (itemsToDisplay > 1) { /* do NOT add \n if last item */
                    /* g_snprintf(myLabelText, 1024, "%s \n", myLabelText); */
                    myLabelText = g_strconcat (myLabelText, " \n", NULL);
                    currentColumn = 0;
                }
                itemsToDisplay--;
            }

            chipFeature ++;
        }

        chipNumber++;
    }

    gtk_label_set_markup(GTK_LABEL(st->panelValuesLabel), myLabelText);
    
    return TRUE;
}


/* create tooltip */
gboolean
sensors_date_tooltip (gpointer data)
{
    /* g_printf(" sensors_date_tooltip \n");  */

    g_return_val_if_fail (data != NULL, FALSE);

    t_sensors *st = (t_sensors *) data;
    
    GtkWidget *widget = st->eventbox;
    
/*      +--------------------+
        I asb-1-100          I
        I   CPU: 42.00       I
        I   MB: 24.50        I
        I eeprom-80          I
        I   id: 42           I
        +--------------------+ */

    /* we will duplicate the string everywhere, 
    so don't allocate a fixed portion.
    FIX ME! */
    gchar *myToolTipText; /* = (gchar *) g_malloc(1024); */
    
    /* circumvent empty char pointer */
    myToolTipText = g_strdup(_("No sensors selected!"));
    
    int i=0;
    
    if (st->sensorNumber > 10) return FALSE;
    
    gboolean first = TRUE;
    
    while ( i < st->sensorNumber ) {
    
        gboolean prependedChipName = FALSE;
    
        int nr1 = 0;
        while ( nr1 < 256 ) {
            
            if ( st->sensorValid[i][nr1] == TRUE &&
                 st->sensorCheckBoxes[i][nr1] == TRUE ) {
                
                if ( prependedChipName != TRUE) {
                    
                    if (first == TRUE) {
                        myToolTipText = g_strdup(st->sensorId[i]);
                        first = FALSE;
                    }
                    else
                        myToolTipText = g_strconcat (
                            myToolTipText, " \n ", st->sensorId[i], NULL);
                        
                    prependedChipName = TRUE;
                }
            
                double sensorFeature;
                int res = sensors_get_feature(*st->chipName[i], nr1, 
                            &sensorFeature);

                if ( res!=0 ) {
                    g_printf( _(" \nXfce Hardware Sensors Plugin: \n \
                      Seems like there was a problem reading a sensor feature \
                        value. \nProper proceeding cannot be guaranteed. \n"));
                    break;
                }
                
                gchar *help = g_strdup_printf("%+5.2f", sensorFeature);

                /* FIXED: '\ ' ain't necessary for spaces. ' ' works. */
                myToolTipText = g_strconcat (myToolTipText, "\n  ", 
                    st->sensorNames[i][nr1], ": ", help, NULL);
                
                st->sensorValues[i][nr1] = g_strdup(help);
                
                g_free(help);
            }

            nr1++;
        }

        i++;
    }

    add_tooltip(widget, myToolTipText);

    return TRUE;
}


/* initialize box and label to pack them together */
void
create_panel_widget (t_sensors * st) {
    /* initialize a new vbox widget */
    st->sensors = gtk_vbox_new(FALSE, 0);
    gtk_widget_show (st->sensors);
    
    /* initialize value label widget */
    st->panelValuesLabel = gtk_label_new(NULL);
    gtk_widget_show (st->panelValuesLabel);
    
    /* create 'valued' label */
    sensors_show_panel(st);
    
    /* add newly created label to box */
    gtk_box_pack_start(GTK_BOX (st->sensors ), st->panelValuesLabel, FALSE, 
        FALSE, 0);
}


static t_sensors *
sensors_new (void)
{
    /* g_printf(" sensors_new\n"); */

    t_sensors *st = g_new (t_sensors, 1);

    /* init xfce sensors stuff */    
    /* this is to be moved to read/write functions! */
    st->showTitle = TRUE;
    st->fontSize = "medium";
    st->fontSizeNumerical = 2;
    st->panelSize=0;
    st->sensorUpdateTime = 60;

    /* init libsensors stuff */
    FILE *filename = fopen("/etc/sensors.conf", "r");
    int sensorsInit = sensors_init(filename);
    if (sensorsInit != 0)
        g_printf("trouble!");
        
    st->sensorNumber=0;
    st->chipName[st->sensorNumber] = 
        sensors_get_detected_chips(&st->sensorNumber);

    /* iterate over chips on mainboard */
    while (st->chipName[st->sensorNumber-1]!=NULL) {
        int currentIndex = st->sensorNumber-1;

        st->sensorId[currentIndex] =  g_strdup_printf("%s-%i-%i", 
                        st->chipName[currentIndex]->prefix, 
                        st->chipName[currentIndex]->bus, 
                        st->chipName[currentIndex]->addr);

        int nr1=0;
        st->sensorsCount[currentIndex]=0;

        /* iterate over chip features, i.e. id, cpu temp, mb temp... */
        while(nr1<256) {
            int res = sensors_get_label(*st->chipName[currentIndex], nr1, 
                                        &st->sensorNames[currentIndex][nr1] );
            if (res==0) {
                double sensorFeature;
                res = sensors_get_feature(*st->chipName[currentIndex], nr1, 
                                          &sensorFeature);
                
                if (res==0) { 
                    gint ci = st->sensorsCount [currentIndex];
                    st->sensorAddress [currentIndex] [ ci ] = nr1;
                    st->sensorsCount [currentIndex]++;
                    st->sensorColors [currentIndex] [nr1] = "#000000";
                    st->sensorValid [currentIndex] [nr1] = TRUE;
                    st->sensorValues [currentIndex] [nr1] = 
                        g_strdup_printf("%+5.2f", sensorFeature);
                    
                }  /* end if sensorFeature */
                else {
                    st->sensorValid [currentIndex] [nr1] = FALSE;
                }

            } /* end if sensorNames */
            
            st->sensorCheckBoxes [currentIndex] [nr1] = FALSE;
            
            nr1++;
        } /* end while nr1 */

        /* static problem if more than 10 sensors ! */
        if (currentIndex>=9) break;

        st->chipName[++currentIndex] = 
            sensors_get_detected_chips(&st->sensorNumber);
    } /* end while sensor chipNames */

    /* decrease sensorNumber which was incremented by last call on
        sensors_get_detected_chips in order to now reflect the correct
        number of found sensor chips */
    st->sensorNumber--;
    
    /* error handling for no sensors */
    if (st->sensorNumber == 0) {
        st->sensorAddress    [0] [0] = 0;
        st->sensorId             [0] =  g_strdup(_("No sensors found!"));
        st->sensorsCount         [0] = 1;
        st->sensorColors     [0] [0] = "#000000";
        st->sensorNames      [0] [0] = "No sensor";
        st->sensorValid      [0] [0] = TRUE;
        st->sensorValues     [0] [0] = g_strdup_printf("%+5.2f", 0.0);
        st->sensorCheckBoxes [0] [0] = FALSE;
    }

    /* create eventbox to catch events on widget */
    st->eventbox = gtk_event_box_new ();
    gtk_widget_set_name (st->eventbox, "xfce_sensors");
    gtk_widget_show (st->eventbox);

    /* Add tooltip to show extended current sensors status */
    sensors_date_tooltip ( (gpointer) st);

    /* fill panel widget with boxes, strings, values, ... */    
    create_panel_widget(st);
   
    /* finally add panel "sensors" to eventbox */
    gtk_container_add (GTK_CONTAINER (st->eventbox), st->sensors);

    /* update tooltip and widget data */
    st->timeout_id = g_timeout_add (st->sensorUpdateTime * 1000, 
                (GtkFunction) sensors_date_tooltip, (gpointer) st);
    
    st->timeout_id2 =g_timeout_add (st->sensorUpdateTime * 1000, 
                (GtkFunction) sensors_show_panel, (gpointer) st);

    return st;
}


static void
sensors_free (Control *control)
{
    /* g_printf(" sensors_free \n"); */

    t_sensors *st = control->data;
    
    /* stop association to libsensors */
    FILE *filename = fopen("/etc/sensors.conf", "r");
    if (filename != NULL)
      {
        int closeResult = fclose(filename);
        if (closeResult!=0) 
           printf(_("A problem occured while trying to close the config file. \
                    Restart your computer ... err ... \
                     restart the sensor daemon only :-) \n"));
      }

    g_return_if_fail (st != NULL);

    /* remove timeout functions */
    if (st->timeout_id)
    	g_source_remove (st->timeout_id);
	
	if (st->timeout_id2)
        g_source_remove (st->timeout_id2);

    /* free structure */
    g_free (st);
}


static void
sensors_attach_callback (Control *control, const char *signal,
		       GCallback callback, gpointer data)
{
/*    g_printf(" sensors_attach_callback \n"); */

    t_sensors *st = control->data;

    g_signal_connect (st->eventbox, signal, callback, data);
}


void
sensors_set_size (Control * control, int size)
{
    /* g_printf(" sensors_set_size  \n"); */

    t_sensors *st = (t_sensors *) control->data;
    st->panelSize = (gint) size;
    
    /* update the panel widget */
    sensors_show_panel((gpointer) st);
}

gint
getIdFromAddress(gint chip, gint addr, t_sensors* st) 
{
    gint id;
    for (id=0; id<st->sensorsCount[chip]; id++) {
        if (addr == st->sensorAddress[chip][id])
            return id;
    }
    
    return (gint) -1;
}

/* Write the configuration at exit */
void
sensors_write_config (Control * control, xmlNodePtr parent)
{
    /* g_printf(" sensors_write_config  \n"); */

    xmlNodePtr root, chipNode, featureNode;
    char value[MAXSTRLEN + 1];

    t_sensors *st = (t_sensors *) control->data;

    /* I use my own node
       It's safer and easier to check
     */
    root = xmlNewTextChild (parent, NULL, "XfceSensors", NULL);
    g_snprintf (value, 2, "%i", st->showTitle);
    xmlSetProp (root, "Show_Title", value);
    
    g_snprintf (value, 8, "%s", st->fontSize);
    xmlSetProp (root, "Font_Size", value);
    
    g_snprintf (value, 2, "%i", st->fontSizeNumerical);
    xmlSetProp (root, "Font_Size_Numerical", value);
    
    g_snprintf (value, 4, "%i", st->sensorUpdateTime);
    xmlSetProp (root, "Update_Interval", value);
    
    int i;
    for (i=0; i<st->sensorNumber; i++) {
        chipNode = xmlNewTextChild (root, NULL, "Chip", NULL);
        g_sprintf (value, "%s", st->sensorId[i]);
        xmlSetProp (chipNode, "Name", value);
        
        /* still limited to 10 sensors totally! */
        g_snprintf (value, 2, "%i", i);
        xmlSetProp (chipNode, "Number", value);
        
        /* only save what was displayed to save time */
        int j;
        
        for (j=0; j<256; j++) {
        
            if (st->sensorCheckBoxes[i][j] == TRUE) {
                featureNode = xmlNewTextChild (chipNode, NULL, "Feature", 
                    NULL);
                
                g_snprintf (value, 4, "%i", getIdFromAddress(i,j, st) );
                xmlSetProp (featureNode, "Id", value);
                
                g_snprintf (value, 4, "%i", j);
                xmlSetProp (featureNode, "Address", value);
                
                g_sprintf (value, "%s", st->sensorNames[i][j]);
                xmlSetProp (featureNode, "Name", value);
                
                g_snprintf (value, 8, "%s", st->sensorColors[i][j]);
                xmlSetProp (featureNode, "Color", value);
                
                g_snprintf (value, 2, "%i", st->sensorCheckBoxes[i][j]);
                xmlSetProp (featureNode, "Show", value);
            }
            
        } /* end for j */
    } /* end for i */
}


/* Read the configuration file at init */
void
sensors_read_config (Control * control, xmlNodePtr node)
{
    /* g_printf(" sensors_read_config  \n"); */

    xmlChar *value;

    t_sensors *st = (t_sensors *) control->data;

    if (!node || !node->children)
	   return;

    node = node->children;

    /* Leave if we can't find the node XfceSensors */
    if (!xmlStrEqual (node->name, "XfceSensors"))
	   return;

    if ((value = xmlGetProp (node, (const xmlChar *) "Show_Title"))) {
        /* g_printf(" value: %s \n", value); */
        st->showTitle = atoi (value);
        g_free (value);
    }

    if ((value = xmlGetProp (node, (const xmlChar *) "Font_Size"))) {
        /* g_printf(" value: %s \n", value); */
        st->fontSize = g_strdup(value);
        g_free (value);
    }
    
    if ((value = xmlGetProp (node, (const xmlChar *) "Font_Size_Numerical"))) {
        /* g_printf(" value: %s \n", value); */
        st->fontSizeNumerical = atoi(value);
        g_free (value);
    }
    
    if ((value = xmlGetProp (node, (const xmlChar *) "Update_Interval"))) {
        /* g_printf(" value: %s \n", value); */
        st->sensorUpdateTime = atoi (value);
        g_free (value);
    }
    
    if (!node)
        return;
    
    xmlNodePtr chipNode = node->children;
    
    while(chipNode) {
        
        /* Leave if we can't find a chip */
        if (!xmlStrEqual (chipNode->name, "Chip"))
    	   return;
    	
    	gchar* sensorName=" ";
    	gint sensorNumber=0;
    	
    	if ((value = xmlGetProp (chipNode, (const xmlChar *) "Name"))) {
            /* g_printf(" value: %s \n", value); */
            sensorName = g_strdup (value);
            g_free (value);
        }
        
        if ((value = xmlGetProp (chipNode, (const xmlChar *) "Number"))) {
            /* g_printf(" value: %s \n", value); */
            sensorNumber = atoi (value);
            g_free (value);
        }
        
        /* assert that file does not contain more information 
          than does exist on system */
        g_return_if_fail(sensorNumber < st->sensorNumber);
        
        /* string comparison */
        if ( *st->sensorId[sensorNumber] == *sensorName ) {
        
            if (!chipNode)
                return;
        
            xmlNodePtr featureNode = chipNode->children;
            
            while(featureNode) {
        
                /* Leave if we can't find a feature */
                if (!xmlStrEqual (featureNode->name, "Feature"))
            	   return;
        	
            	gint id=0, address=0;
        	
            	if ((value = 
            	   xmlGetProp (featureNode, (const xmlChar *) "Id"))) {
                    /* g_printf(" value: %s \n", value); */
                    id = atoi (value);
                    g_free (value);
                }
            
                if ((value = 
                    xmlGetProp (featureNode, (const xmlChar *) "Address"))) {
                    /* g_printf(" value: %s \n", value); */
                    address = atoi (value);
                    g_free (value);
                }
                
                /* assert correctly saved file */
                g_return_if_fail 
                    (st->sensorAddress[sensorNumber][id] == address);
            
                if ((value = 
                    xmlGetProp (featureNode, (const xmlChar *) "Name"))) {
                    /* g_printf(" value: %s \n", value); */
                    st->sensorNames[sensorNumber][address] = g_strdup (value);
                    g_free (value);
                }

                if ((value = 
                    xmlGetProp (featureNode, (const xmlChar *) "Color"))) {
                    /* g_printf(" value: %s \n", value); */
                    st->sensorColors[sensorNumber][address] = g_strdup (value);
                    g_free (value);
                }
                
                if ((value = 
                    xmlGetProp (featureNode, (const xmlChar *) "Show"))) {
                    /* g_printf(" value: %s \n", value); */
                    st->sensorCheckBoxes[sensorNumber][address] = atoi (value);
                    g_free (value);
                }
                    
                featureNode = featureNode->next;
            }
        }
        
        chipNode = chipNode->next;
    	
    	g_free(sensorName);
	
	}

    /* Try to resize the sensors to fit the user settings.
       Maybe calling sensors_show_panel() would suffice. */
    sensors_set_size (control, settings.size);
    
    /* do also modify the tooltip text */
    sensors_date_tooltip((gpointer) st);
}


static void
show_title_toggled (  GtkWidget *widget, SensorsDialog *sd )
{
    sd->sensors->showTitle = gtk_toggle_button_get_active
        ( GTK_TOGGLE_BUTTON(widget) );
    sensors_show_panel((gpointer) sd->sensors);
    /* g_printf(" show_title_toggled: %i \n", sd->sensors->showTitle); */
}


static void 
sensor_entry_changed (  GtkWidget *widget, SensorsDialog *sd )
{
/*    g_printf(" sensor_entry_changed \n"); */

    gint gtk_combo_box_active = 
        gtk_combo_box_get_active(GTK_COMBO_BOX (widget));

    /* widget should be sd->myComboBox */
    gtk_label_set_label (GTK_LABEL(sd->mySensorLabel), 
            (const gchar*) sensors_get_adapter_name
                (sd->sensors->chipName[gtk_combo_box_active]->bus) );
    gtk_frame_set_label( GTK_FRAME (sd->myFrame), 
            sd->sensors->sensorId [ gtk_combo_box_active ] );
    
    gtk_tree_view_set_model ( 
        GTK_TREE_VIEW (sd->myTreeView), 
        GTK_TREE_MODEL (sd->myListStore [ gtk_combo_box_active ]) );
}


static void
gtk_font_size_change  ( GtkWidget *widget, SensorsDialog *sd )
{
/*    g_printf(" gtk_font_size_change \n"); */

    switch ( gtk_combo_box_get_active(GTK_COMBO_BOX(widget)) ) {
    
        case 0: sd->sensors->fontSize = "x-small"; break;
        case 1: sd->sensors->fontSize = "small"; break;
        case 3: sd->sensors->fontSize = "large"; break;
        case 4: sd->sensors->fontSize = "x-large"; break;
        default: sd->sensors->fontSize = "medium";
    }
    
    sd->sensors->fontSizeNumerical = 
        gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
    
    /* refresh the panel content */
    sensors_show_panel((gpointer) sd->sensors);
}


static void
adjustment_value_changed ( GtkWidget *widget, SensorsDialog* sd)
{
/*    g_printf(" gtk_adjustment_value_changed \n"); */
    
    sd->sensors->sensorUpdateTime = 
        (gint) gtk_adjustment_get_value ( GTK_ADJUSTMENT (widget) );
    
    /* stop the timeout functions ... */
    g_source_remove (sd->sensors->timeout_id);
    g_source_remove (sd->sensors->timeout_id2);
    /* ... and start them again */
    sd->sensors->timeout_id  = g_timeout_add ( 
        sd->sensors->sensorUpdateTime * 1000, 
        (GtkFunction) sensors_date_tooltip, (gpointer) sd->sensors);
    sd->sensors->timeout_id2 = g_timeout_add ( 
        sd->sensors->sensorUpdateTime * 1000, 
        (GtkFunction) sensors_show_panel, (gpointer) sd->sensors);
}


static void
gtk_cell_color_edited (GtkCellRendererText *cellrenderertext, gchar *path_str, 
                       gchar *new_color, SensorsDialog *sd)
{
/*    g_printf(" gtk_cell_color_edited \n");  */

    /* store new color in appropriate array */
    gboolean hexColor = g_str_has_prefix (new_color, "#");
    
    if (hexColor && strlen(new_color) == 7) {
        int i;
        for (i=1; i<7; i++) {
            /* only save hex numbers! */
            if ( ! g_ascii_isxdigit (new_color[i]) ) 
                return; 
        }
        
        gint gtk_combo_box_active = 
            gtk_combo_box_get_active(GTK_COMBO_BOX (sd->myComboBox));
        
        /* get model and path */
        GtkTreeModel *model = (GtkTreeModel *) sd->myListStore
            [gtk_combo_box_active];
        GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
        
        /* get model iterator */
        GtkTreeIter iter;
        gtk_tree_model_get_iter (model, &iter, path);
        
        /* set new value */
        gtk_tree_store_set (GTK_TREE_STORE (model), &iter, 3, new_color, -1);
        int help = sd->sensors->sensorAddress  [gtk_combo_box_active]
                                               [atoi(path_str)];
        sd->sensors->sensorColors [gtk_combo_box_active] [help] = 
            g_strdup(new_color);
                        
        /* clean up */
        gtk_tree_path_free (path);
        
        /* update panel */
        sensors_show_panel((gpointer) sd->sensors);
    }
}


static void
gtk_cell_text_edited (GtkCellRendererText *cellrenderertext, 
                      gchar *path_str, gchar *new_text, SensorsDialog *sd)
{
/*    g_printf(" gtk_cell_text_edited \n"); */

    gint gtk_combo_box_active = 
        gtk_combo_box_get_active(GTK_COMBO_BOX (sd->myComboBox));

    GtkTreeModel *model = 
        (GtkTreeModel *) sd->myListStore [gtk_combo_box_active];
    GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
    GtkTreeIter iter;
    
    /* get model iterator */
    gtk_tree_model_get_iter (model, &iter, path);
    
    /* set new value */
    gtk_tree_store_set (GTK_TREE_STORE (model), &iter, 0, new_text, -1);
    int help = 
        sd->sensors->sensorAddress [ gtk_combo_box_active ] [atoi(path_str)];
    sd->sensors->sensorNames[ gtk_combo_box_active ] [ help ] = 
        g_strdup(new_text);
                        
    /* clean up */
    gtk_tree_path_free (path);
    
    sensors_date_tooltip ((gpointer) sd->sensors);
}


static void
gtk_cell_toggle ( GtkCellRendererToggle *cell, gchar *path_str, 
                  SensorsDialog *sd)
{
/*    g_printf(" gtk_cell_toggle \n"); */

    gint gtk_combo_box_active = 
        gtk_combo_box_get_active(GTK_COMBO_BOX (sd->myComboBox));

    GtkTreeModel *model = 
        (GtkTreeModel *) sd->myListStore[gtk_combo_box_active];
    GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
    GtkTreeIter iter;
    gboolean toggle_item;

    /* get toggled iter */
    gtk_tree_model_get_iter (model, &iter, path);
    gtk_tree_model_get (model, &iter, 2, &toggle_item, -1);
     
    /* do something with the value */
    toggle_item ^= 1;
         
    /* set new value */
    gtk_tree_store_set (GTK_TREE_STORE (model), &iter, 2, toggle_item, -1);
    int help = 
        sd->sensors->sensorAddress [ gtk_combo_box_active ] [atoi(path_str)];
    
    sd->sensors->sensorCheckBoxes [gtk_combo_box_active ] [ help ] = 
        toggle_item;
                        
    /* clean up */
    gtk_tree_path_free (path);
    
    /* update tooltip and panel widget */
    sensors_date_tooltip ((gpointer) sd->sensors);
    sensors_show_panel((gpointer) sd->sensors);
}


void
init_widgets (SensorsDialog *sd)
{
    /* g_printf(" init_widgets\n");  */
    
    int i=0;
    while( i < sd->sensors->sensorNumber ) {
        sd->myListStore[i] = gtk_tree_store_new (4, G_TYPE_STRING, 
                        G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_STRING);
        i++;
    }
    
    GtkTreeIter iter;
    
    i=0;
    while( i < sd->sensors->sensorNumber ) {
        
        gtk_combo_box_append_text ( GTK_COMBO_BOX(sd->myComboBox), 
                                    sd->sensors->sensorId[i] );
        
        int nr1=0;

        while( nr1 < 256 ) {
            if ( sd->sensors->sensorValid[i][nr1] == TRUE ) {
                double sensorFeature;
                int res = sensors_get_feature
                        (*sd->sensors->chipName[i], nr1, &sensorFeature);

                if ( res!=0) {
                    g_printf( _(" \nXfce Hardware Sensors Plugin: \
                        \nSeems like there was a problem reading a sensor \
                        feature value. \nProper proceeding cannot be \
                        guaranteed.\n") );
                    break;
                }
                sd->sensors->sensorValues[i][nr1] = 
                    g_strdup_printf("%+5.2f", sensorFeature);
                gtk_tree_store_append ( GTK_TREE_STORE (sd->myListStore[i]), 
                                        &iter, NULL);
                gtk_tree_store_set ( GTK_TREE_STORE (sd->myListStore[i]),
                                     &iter,
                                     0, sd->sensors->sensorNames[i][nr1] ,
                                     1, sd->sensors->sensorValues[i][nr1] ,
                                     2, sd->sensors->sensorCheckBoxes[i][nr1],
                                     3, sd->sensors->sensorColors[i][nr1], -1);
            } /* end if sensors-valid */

            nr1++;
        } /* end while nr1 */

        i++;
    } /* end while i < sensorNumber */
    
    if(sd->sensors->sensorNumber == 0) {
        gtk_combo_box_append_text ( GTK_COMBO_BOX(sd->myComboBox), 
                                sd->sensors->sensorId[0] );
        sd->myListStore[0] = gtk_tree_store_new (4, G_TYPE_STRING, 
                            G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_STRING);
        sd->sensors->sensorValues[0][0] = 
                            g_strdup_printf("%+5.2f", 0.0);
        gtk_tree_store_append ( GTK_TREE_STORE (sd->myListStore[0]), 
                            &iter, NULL);
        gtk_tree_store_set ( GTK_TREE_STORE (sd->myListStore[0]),
                            &iter,
                            0, sd->sensors->sensorNames[0][0] ,
                            1, sd->sensors->sensorValues[0][0] ,
                            2, sd->sensors->sensorCheckBoxes[0][0],
                            3, sd->sensors->sensorColors[0][0], -1);
    }
    
}


static void
add_title_box (GtkWidget * vbox, GtkSizeGroup * sg, SensorsDialog * sd)
{
    /* g_printf(" add_title_box \n"); */

    GtkWidget *hbox, *checkButton;

    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);

    checkButton = gtk_check_button_new_with_label (_("Show title"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(checkButton), 
                                  sd->sensors->showTitle);
    gtk_widget_show (checkButton);
    gtk_size_group_add_widget (sg, checkButton);
    
    gtk_box_pack_start (GTK_BOX (hbox), checkButton, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);
        
    g_signal_connect (G_OBJECT (checkButton), "toggled", 
                      G_CALLBACK (show_title_toggled), sd );
}


static void
add_type_box (GtkWidget * vbox, GtkSizeGroup * sg, SensorsDialog * sd)
{
    /* g_printf(" add_type_box \n"); */

    GtkWidget *hbox, *label;

    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

    label = gtk_label_new (_("Sensors type:"));
    gtk_widget_show (label);
    gtk_size_group_add_widget (sg, label);
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    
    gtk_widget_show (sd->myComboBox);
    gtk_box_pack_start (GTK_BOX (hbox), sd->myComboBox, FALSE, FALSE, 0);
    
    g_signal_connect (G_OBJECT (sd->myComboBox), "changed", 
                      G_CALLBACK (sensor_entry_changed), sd );
}


static void
add_sensor_settings_box ( GtkWidget * vbox, GtkSizeGroup * sg, 
                          SensorsDialog * sd)
{
   /* g_printf(" add_sensor_settings_box\n"); */
    /*
     * Tree View & Model stuff
     *                               */
    GtkTreeViewColumn *aTreeViewColumn;
    GtkWidget *myVboxFrame;
    GtkCellRenderer *myCellRendererText;
    
    gint gtk_combo_box_active = 
        gtk_combo_box_get_active(GTK_COMBO_BOX(sd->myComboBox));
    
    sd->myFrame = gtk_frame_new
        ( sd->sensors->sensorId [gtk_combo_box_active ] );
    
    if (sd->sensors->sensorNumber >0)
        sd->mySensorLabel = gtk_label_new 
            ( sensors_get_adapter_name
                ( sd->sensors->chipName[gtk_combo_box_active ]->bus) );
    else
        sd->mySensorLabel = 
            gtk_label_new ( sd->sensors->sensorId [gtk_combo_box_active ] );
        
    sd->myTreeView = gtk_tree_view_new_with_model
        ( GTK_TREE_MODEL ( sd->myListStore[ gtk_combo_box_active ] ) );

    myCellRendererText = gtk_cell_renderer_text_new ();
    g_object_set ( (gpointer*) myCellRendererText, "editable", TRUE, NULL );

    aTreeViewColumn = gtk_tree_view_column_new_with_attributes ("Name", 
                        myCellRendererText, "text", 0, NULL);
    g_signal_connect    (G_OBJECT (myCellRendererText), "edited", 
                        G_CALLBACK (gtk_cell_text_edited), sd);
    gtk_tree_view_column_set_expand (aTreeViewColumn, TRUE);
    gtk_tree_view_append_column (GTK_TREE_VIEW (sd->myTreeView), 
                        GTK_TREE_VIEW_COLUMN (aTreeViewColumn));
    
    myCellRendererText = gtk_cell_renderer_text_new ();
    aTreeViewColumn = gtk_tree_view_column_new_with_attributes ("Value", 
                        myCellRendererText, "text", 1, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (sd->myTreeView), 
                        GTK_TREE_VIEW_COLUMN (aTreeViewColumn));
    
    GtkCellRenderer *myCellRendererToggle = gtk_cell_renderer_toggle_new();
    aTreeViewColumn = gtk_tree_view_column_new_with_attributes ("Show", 
                        myCellRendererToggle, "active", 2, NULL);
    g_signal_connect    (G_OBJECT (myCellRendererToggle), "toggled", 
                        G_CALLBACK (gtk_cell_toggle), sd );
    gtk_tree_view_append_column (GTK_TREE_VIEW (sd->myTreeView), 
                        GTK_TREE_VIEW_COLUMN (aTreeViewColumn));
    
    myCellRendererText = gtk_cell_renderer_text_new ();
    g_object_set ( (gpointer*) myCellRendererText, "editable", TRUE, NULL );
    aTreeViewColumn = gtk_tree_view_column_new_with_attributes ("Color", 
                        myCellRendererText, "text", 3, NULL);
    g_signal_connect    (G_OBJECT (myCellRendererText), "edited", 
                        G_CALLBACK (gtk_cell_color_edited), sd);
    gtk_tree_view_append_column (GTK_TREE_VIEW (sd->myTreeView), 
                        GTK_TREE_VIEW_COLUMN (aTreeViewColumn));

    /*    
     * Gtk Widget stuff 
     *                       */
    GtkWidget *myScrolledWindow = gtk_scrolled_window_new (NULL, NULL);
        gtk_scrolled_window_set_policy (
            GTK_SCROLLED_WINDOW (myScrolledWindow), GTK_POLICY_AUTOMATIC, 
            GTK_POLICY_AUTOMATIC);
        gtk_container_set_border_width (GTK_CONTAINER (myScrolledWindow), 4);
        gtk_scrolled_window_add_with_viewport (
            GTK_SCROLLED_WINDOW (myScrolledWindow), sd->myTreeView);

    myVboxFrame = gtk_vbox_new(FALSE, 4);

    gtk_box_pack_start (GTK_BOX (myVboxFrame), sd->mySensorLabel, FALSE, FALSE,
        4);
    gtk_box_pack_start (GTK_BOX (myVboxFrame), myScrolledWindow, TRUE, TRUE, 
        0);

    gtk_container_add (GTK_CONTAINER (sd->myFrame), myVboxFrame);

    gtk_box_pack_start (GTK_BOX (vbox), sd->myFrame, TRUE, TRUE, 0);

    gtk_widget_show (sd->myTreeView);
    gtk_widget_show (myVboxFrame);
    gtk_widget_show (sd->myFrame);
    gtk_widget_show (myScrolledWindow);
    gtk_widget_show (sd->mySensorLabel);
}


static void
add_font_size_box (GtkWidget * vbox, GtkSizeGroup * sg, SensorsDialog * sd)
{
    /* g_printf(" add_font_size_box\n"); */

    GtkWidget *myFontLabel = gtk_label_new (_("Font size:"));
    GtkWidget *myFontBox = gtk_hbox_new(FALSE, 0);
    GtkWidget *myFontSizeComboBox = gtk_combo_box_new_text();

    gtk_combo_box_append_text(GTK_COMBO_BOX(myFontSizeComboBox), _("x-small"));
    gtk_combo_box_append_text(GTK_COMBO_BOX(myFontSizeComboBox), _("small")  );
    gtk_combo_box_append_text(GTK_COMBO_BOX(myFontSizeComboBox), _("medium") );
    gtk_combo_box_append_text(GTK_COMBO_BOX(myFontSizeComboBox), _("large")  );
    gtk_combo_box_append_text(GTK_COMBO_BOX(myFontSizeComboBox), _("x-large"));
    gtk_combo_box_set_active (GTK_COMBO_BOX(myFontSizeComboBox), 
        sd->sensors->fontSizeNumerical);

    gtk_box_pack_start (GTK_BOX (myFontBox), myFontLabel, FALSE, FALSE, 2);
    gtk_box_pack_start (GTK_BOX (myFontBox), myFontSizeComboBox, FALSE, FALSE,
        2);    
    gtk_box_pack_start (GTK_BOX (vbox), myFontBox, FALSE, FALSE, 0);
    
    gtk_widget_show (myFontLabel);
    gtk_widget_show (myFontSizeComboBox);
    gtk_widget_show (myFontBox);
    
    g_signal_connect   (G_OBJECT (myFontSizeComboBox), "changed", 
                        G_CALLBACK (gtk_font_size_change), sd );
}


static void
add_update_time_box (GtkWidget * vbox, GtkSizeGroup * sg, SensorsDialog * sd)
{
    /* g_printf(" add_update_time_box\n"); */
    
    GtkWidget *spinner, *myLabel, *myBox;
    GtkAdjustment *spinner_adj;

    spinner_adj = (GtkAdjustment *) gtk_adjustment_new (
        sd->sensors->sensorUpdateTime, 10.0, 990.0, 10.0, 60.0, 60.0);
   
   
    /* creates the spinner, with no decimal places */
    spinner = gtk_spin_button_new (spinner_adj, 10.0, 0);

    myLabel = gtk_label_new (_("Update interval (seconds):"));
    myBox = gtk_hbox_new(FALSE, 0);

    gtk_box_pack_start (GTK_BOX (myBox), myLabel, FALSE, FALSE, 2);
    gtk_box_pack_start (GTK_BOX (myBox), spinner, FALSE, FALSE, 2);    
    gtk_box_pack_start (GTK_BOX (vbox), myBox, FALSE, FALSE, 0);
    
    gtk_widget_show (myLabel);
    gtk_widget_show (spinner);
    gtk_widget_show (myBox);
    
    g_signal_connect   (G_OBJECT (spinner_adj), "value_changed",
                        G_CALLBACK (adjustment_value_changed), sd );
}


/* create sensor options box */
void
sensors_create_options (Control *control, GtkContainer *container,
		      GtkWidget *done)
{
/*    g_printf(" sensors_create_options\n"); */

    GtkWidget *vbox;
    GtkSizeGroup *sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
    SensorsDialog *sd;

    xfce_textdomain (GETTEXT_PACKAGE, LOCALEDIR, "UTF-8");

    sd = g_new0 (SensorsDialog, 1);

    sd->sensors = control->data;
    sd->dialog = gtk_widget_get_toplevel (done);

    g_signal_connect_swapped (sd->dialog, "destroy-event", 
                              G_CALLBACK (g_free), sd);

    /* the options box */
    vbox = gtk_vbox_new (FALSE, BORDER);
    gtk_widget_show (vbox);
    
    sd->myComboBox = gtk_combo_box_new_text();
        
    init_widgets(sd);
    
    gtk_combo_box_set_active(GTK_COMBO_BOX(sd->myComboBox), 0);

    add_title_box (vbox, sg, sd);

    add_type_box (vbox, sg, sd);

    add_sensor_settings_box (vbox, sg, sd);

    add_font_size_box (vbox, sg, sd);
    
    add_update_time_box(vbox, sg, sd);
    
    gtk_widget_set_size_request (vbox, 315, 250);

    gtk_container_add (container, vbox);
}


/*  Sensors panel control
 *  ---------------------
 */
gboolean
create_sensors_control (Control * control)
{
/*    g_printf(" create_sensors_control\n"); */

    t_sensors *sensors = sensors_new ();

    gtk_container_add (GTK_CONTAINER (control->base), sensors->eventbox);

    control->data = (gpointer) sensors;
    control->with_popup = FALSE;

    gtk_widget_set_size_request (control->base, -1, -1);
    sensors_set_size (control, settings.size);

    return TRUE;
}


G_MODULE_EXPORT void
xfce_control_class_init (ControlClass * cc)
{
/*    g_printf(" xfce_control_class_init\n"); */

    xfce_textdomain (GETTEXT_PACKAGE, LOCALEDIR, "UTF-8");

    cc->name = "sensors";
    cc->caption = _("Hardware Sensors");

    cc->create_control = (CreateControlFunc) create_sensors_control;

    cc->free = sensors_free;
    cc->read_config = sensors_read_config;
    cc->write_config = sensors_write_config;

    cc->attach_callback = sensors_attach_callback;

    cc->create_options = sensors_create_options;

    cc->set_size = sensors_set_size;

    control_class_set_unique (cc, TRUE);
}

/* macro defined in plugins.h */
XFCE_PLUGIN_CHECK_INIT
