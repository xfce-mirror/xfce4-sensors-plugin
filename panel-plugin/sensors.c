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

/* This plugin requires libsensors-3 and its headers !*/

/* Note for programmers and editors: Try to use 4 spaces instead of Tab! */


#include "sensors.h"

/* #define DEBUG */

static void
sensors_set_bar_size (GtkWidget *bar, int size, int orientation)
{
    #ifdef DEBUG
    g_printf ("sensors_set_bar_size \n");
    #endif
	/* check arguments */
	g_return_if_fail (G_IS_OBJECT(bar));
	
	int sizeClass;
	
	if (size<22) sizeClass = 2; 
	else if (size < 32) sizeClass = 3;
	else if (size < 48) sizeClass = 4;
	else if (size < 64) sizeClass = 5;
	else sizeClass = 6;

	if (orientation == GTK_ORIENTATION_HORIZONTAL) {
		gtk_widget_set_size_request (bar, 2 * sizeClass, size - 4);
	} else {
		gtk_widget_set_size_request (bar, size - 4, 2 * sizeClass);
	}
}


static void
sensors_set_bar_color (GtkWidget *bar, double fraction)
{
    #ifdef DEBUG
    g_printf ("sensors_set_bar_color \n");
    #endif
	/* check arguments */
	g_return_if_fail(G_IS_OBJECT(bar));

	GtkRcStyle *rc = gtk_widget_get_modifier_style(GTK_WIDGET(bar));
	if (!rc) {
		rc = gtk_rc_style_new();
	}
	GdkColor color;
	if (fraction >= 1) {
		gdk_color_parse(COLOR_ERROR, &color);
	} else if ((fraction < .2) || (fraction > .8)) {
		gdk_color_parse(COLOR_WARN, &color);
	} else {
		gdk_color_parse(COLOR_NORMAL, &color);
	}
	rc->color_flags[GTK_STATE_PRELIGHT] |= GTK_RC_BG;
	rc->bg[GTK_STATE_PRELIGHT] = color;
	gtk_widget_modify_bg(bar, GTK_STATE_PRELIGHT, &color);
}

static double
sensors_get_percentage (gint chipNum, gint feature, t_sensors *st)
{
    #ifdef DEBUG
    g_printf ("sensors_get_percentage \n");
    #endif
	int value = st->sensorRawValues[chipNum][feature];
	int min = st->sensorMinValues[chipNum][feature];
	int max = st->sensorMaxValues[chipNum][feature];
	double percentage = (double) (value - min) / (max - min);
	if ((percentage > 1) || (percentage <= 0)) {
		return 1;
	}
	return percentage;
}


static void
sensors_remove_graphical_panel (t_sensors *st)
{
    #ifdef DEBUG
    g_printf ("sensors_remove_graphical_panel \n");
    #endif
	int chip, feature;
	for (chip=0; chip < st->sensorNumber; chip++) {
		for (feature=0; feature < FEATURES_PER_SENSOR; feature++) {
			if (st->sensorCheckBoxes[chip][feature] == TRUE) {
				t_barpanel *panel = (t_barpanel*) st->panels[chip][feature];

				gtk_widget_destroy(panel->progressbar);
				gtk_widget_destroy(panel->label);
				gtk_widget_destroy(panel->databox);
				g_free(panel);
			}
		}
	}
	st->barsCreated = FALSE;
	gtk_widget_hide(st->panelValuesLabel);
}


static void
sensors_update_graphical_panel (t_sensors *st)
{
    #ifdef DEBUG
    g_printf ("sensors_update_graphical_panel \n");
    #endif
	int chip, feature;
	for (chip=0; chip < st->sensorNumber; chip++) {
		for (feature=0; feature < FEATURES_PER_SENSOR; feature++) {
			if (st->sensorCheckBoxes[chip][feature] == TRUE) {
				t_barpanel *panel = (t_barpanel*) st->panels[chip][feature];

				GtkWidget *bar = panel->progressbar;
				g_return_if_fail (G_IS_OBJECT(bar));

				sensors_set_bar_size (bar, (int) st->panelSize,
							st->orientation);
				double fraction = sensors_get_percentage (
							chip, feature, st);
				sensors_set_bar_color (bar, fraction);
				gtk_progress_bar_set_fraction (
					GTK_PROGRESS_BAR(bar), fraction);
			}
		}
	}
}


static void
sensors_add_graphical_panel (t_sensors *st)
{
    #ifdef DEBUG
    g_printf ("sensors_add_graphical_panel \n");
    #endif
    gtk_label_set_markup(GTK_LABEL(st->panelValuesLabel), _("<b>Sensors</b>"));

	gboolean has_bars = FALSE;

	int chip, feature;
	for (chip=0; chip < st->sensorNumber; chip++) {
		for (feature=0; feature < FEATURES_PER_SENSOR; feature++) {
			if (st->sensorCheckBoxes[chip][feature] == TRUE) {
				has_bars = TRUE;

				/* prepare the progress bar */
				GtkWidget *progbar = gtk_progress_bar_new();

				if (st->orientation == GTK_ORIENTATION_HORIZONTAL) {
					gtk_progress_bar_set_orientation (
						GTK_PROGRESS_BAR (progbar),
						GTK_PROGRESS_BOTTOM_TO_TOP);
				} else {
					gtk_progress_bar_set_orientation (
						GTK_PROGRESS_BAR (progbar),
						GTK_PROGRESS_LEFT_TO_RIGHT);
				}
				sensors_set_bar_size (progbar,
						(int) st->panelSize,
						st->orientation);
				gtk_widget_show(progbar);

				/* put it all in the box */
				GtkWidget *databox;
				if (st->orientation == GTK_ORIENTATION_HORIZONTAL) {
					databox = gtk_hbox_new (FALSE, 0);
				} else {
					databox = gtk_vbox_new (FALSE, 0);
				}
				gtk_widget_show (databox);
				
				/* save the panel elements */
				t_barpanel *panel = g_new (t_barpanel, 1);
				panel->progressbar = progbar;
				
				/* create the label stuff only if needed - saves some memory! */
				if (st->showLabels == TRUE) {
                    gchar caption[128];
                    g_snprintf (caption, sizeof(caption), "%s",
                    	st->sensorNames[chip][feature]);
                    GtkWidget *label = gtk_label_new (caption);
                    gtk_misc_set_padding (GTK_MISC(label), 3, 0);
                    gtk_widget_show (label);
                    panel->label = label;
                    
                    gtk_box_pack_start (GTK_BOX(databox), label,
							FALSE, FALSE, 0);
			    }
			    else {
			        panel->label = NULL;
			    }
				
				gtk_box_pack_start (GTK_BOX(databox), progbar,
							FALSE, FALSE, 0);
				gtk_container_set_border_width (
					GTK_CONTAINER(databox), BORDER); /* border_width); */

				panel->databox = databox;
				st->panels[chip][feature] = (GtkWidget*) panel;

				/* and add it to the outer box */
				gtk_box_pack_start (GTK_BOX (st->sensors),
						databox, FALSE, FALSE, 0);
			}
		}
	}
	if (has_bars && !st->showTitle) {
		gtk_widget_hide (st->panelValuesLabel);
	} else {
		gtk_widget_show (st->panelValuesLabel);
	}
	st->barsCreated = TRUE;
	sensors_update_graphical_panel(st);
}


static gboolean
sensors_show_graphical_panel (t_sensors *st)
{
    #ifdef DEBUG
    g_printf ("sensors_show_graphical_panel \n");
    #endif
	if (st->barsCreated == TRUE) {
		sensors_update_graphical_panel(st);
	} else {
		sensors_add_graphical_panel(st);
	}
	return TRUE;
}


/* draw label with sensor values into panel's vbox */
static gboolean
sensors_show_text_panel (t_sensors *st) 
{
    #ifdef DEBUG
    g_printf ("sensors_show_text_panel \n");
    #endif
/* REMARK:
    Since sensors_show_panel is called with the same period as
    update_tooltip and update_tooltip already reads in new
    sensor values, this isn't done again in here */

    gtk_widget_show (st->panelValuesLabel);
    gtk_misc_set_padding (GTK_MISC(st->panelValuesLabel), 2, 0);

    /* add labels */
    gint chipNumber = 0;
    gint itemsToDisplay = 0;
    
    gchar *myLabelText;
    
    if (st->showTitle == TRUE) {
        myLabelText = g_strdup_printf(_(
          "<span foreground=\"#000000\" size=\"%s\"><b>Sensors</b></span>\n"), 
          st->fontSize);
    }
    else /* nul-terminate the string for further concatenating */
        myLabelText = g_strdup ("");
    
    /* count number of checked sensors to display.
       this could also be done by every toggle/untoggle action
       by putting this variable into t_sensors */
    while (chipNumber < st->sensorNumber) {
    
        gint chipFeature = 0;
    
        while (chipFeature < FEATURES_PER_SENSOR) {
        
            if (st->sensorCheckBoxes[chipNumber][chipFeature] == TRUE)
                itemsToDisplay++;

            chipFeature ++;
        }

        chipNumber++;
    }
    
    gint numRows = 0, numCols, size = st->panelSize;
    
    if (st->orientation == GTK_ORIENTATION_HORIZONTAL) {
        switch (st->fontSizeNumerical) {
            /* we have to allocate N items per row and N-1 spaces. Assuming that
               row spacing is half the font size and smallest font size is 6, we add
               6/2 more space for row spacing per each item, except for the last.
               this one is added to the panelSize. */
            /* The author knows that normally the font sizes depend on the settings
               of the icon theme, but he doesn't want to find out where those
               settings can be found. */
            case 0: for (size+=3; size>=(6+3); numRows++, size-=(6+3)) ; break;
            
            case 1: for (size+=4; size>=(8+4); numRows++, size-=(8+4)) ; break;
            
            case 2: for (size+=5; size>=(10+5); numRows++, size-=(10+5)) ; break;
            
            case 3: for (size+=6; size>=(12+6); numRows++, size-=(12+6)) ; break;
            
            case 4: for (size+=7; size>=(14+7); numRows++, size-=(14+7)) ; break;
            default: numRows = 2;
        } /* end switch */
    } /* end if horizontal */
    else {
        numRows = FEATURES_PER_SENSOR * SENSORS + 1; /* cannot have more rows ;-) */
    }
    
    /* fail-safe */
    if (numRows==0) numRows = 1;    
    
    if (st->showTitle == TRUE ) {
        numRows--; /* we can draw one more item per column */
        
        /* draw the title if no item is to be displayed */
        /* This allows a user to still find the plugin */
        /* might also be replaced by approximately two spaces */
        if (itemsToDisplay==0) 
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
    
        while (chipFeature < FEATURES_PER_SENSOR) {
        
            if (st->sensorCheckBoxes[chipNumber][chipFeature] == TRUE) {
                myLabelText = g_strconcat (myLabelText, "<span foreground=\"", 
                    st->sensorColors[chipNumber][chipFeature], "\" size=\"", 
                    st->fontSize, "\">",
                    st->sensorValues[chipNumber][chipFeature], "</span>", 
                    NULL);
            
                if (currentColumn < numCols-1) {
                    myLabelText = g_strconcat (myLabelText, " \t", NULL);
                    currentColumn++;
                }
                else if (itemsToDisplay > 1) { /* do NOT add \n if last item */
                    myLabelText = g_strconcat (myLabelText, " \n", NULL);
                    currentColumn = 0;
                }
                itemsToDisplay--;
            }

            chipFeature++;
        }

        chipNumber++;
    }

    gtk_label_set_markup (GTK_LABEL(st->panelValuesLabel), myLabelText);
    
    return TRUE;
}


/* create tooltip */
static gboolean
sensors_date_tooltip (gpointer data)
{
    #ifdef DEBUG
    g_printf ("sensors_date_tooltip \n");
    #endif

    g_return_val_if_fail (data != NULL, FALSE);

    t_sensors *st = (t_sensors *) data;
    
    GtkWidget *widget = st->eventbox;

/* FIXME: Work more with asprintf(target, source); here */

    gchar *myToolTipText; 
    
    /* circumvent empty char pointer */
    myToolTipText = g_strdup (_("No sensors selected!"));
    
    int i=0;
    
    if (st->sensorNumber > SENSORS) return FALSE;
    
    gboolean first = TRUE;
    
    while ( i < st->sensorNumber ) {
    
        gboolean prependedChipName = FALSE;
    
        int nr1 = 0;
        while ( nr1 < FEATURES_PER_SENSOR ) {
            
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
                    g_printf( _(" \nXfce Hardware Sensors Plugin: \
\nSeems like there was a problem reading a sensor \
feature value. \nProper proceeding cannot be \
guaranteed.\n"));
                    break;
                }
                
                gchar *help;
                switch (st->sensor_types[i][nr1]) {
                  case TEMPERATURE:
                           if( st->scale == FAHRENHEIT ) {
                               help = g_strdup_printf("%5.1f °F", ((float)sensorFeature)*9/5+32);
			   } else { /* Celsius */
                               help = g_strdup_printf("%5.1f °C", sensorFeature);
                           }
                           break;
                  case VOLTAGE: 
                           help = g_strdup_printf("%+5.2f V", sensorFeature);
                           break;
                  case SPEED: 
                           help = g_strdup_printf(_("%5.0f rpm"), sensorFeature);
                           break;
                  default: help = g_strdup_printf("%+5.2f", sensorFeature);
                           break;
                }
                
                myToolTipText = g_strconcat (myToolTipText, "\n  ", 
                    st->sensorNames[i][nr1], ": ", help, NULL);
                
                st->sensorValues[i][nr1] = g_strdup (help);
		        st->sensorRawValues[i][nr1] = sensorFeature;
                
                g_free (help);
            }

            nr1++;
        }

        i++;
    }

    if (!tooltips) {
      tooltips = gtk_tooltips_new();
    }
    gtk_tooltips_set_tip (tooltips, GTK_WIDGET(st->eventbox), myToolTipText, 
                          NULL);

    return TRUE;
}


static gboolean
sensors_show_panel (gpointer data)
{
    #ifdef DEBUG
    g_printf ("sensors_show_panel \n");
    #endif
	g_return_val_if_fail (data != NULL, FALSE);

	t_sensors *st = (t_sensors *) data;

	sensors_date_tooltip ((gpointer) st);
	if (st->useBarUI == FALSE) {
		return sensors_show_text_panel (st);
	} else {
		return sensors_show_graphical_panel (st);
	}
}


static void
sensors_set_orientation (XfcePanelPlugin *plugin, GtkOrientation orientation, 
                                     t_sensors *st)
{
    #ifdef DEBUG
    g_printf ("sensors_set_orientation \n");
    #endif

	if (orientation == st->orientation) return;
	
	st->orientation = orientation;
	
	if (st->useBarUI==FALSE) return;
	
	GtkWidget *newBox;

	if (orientation == GTK_ORIENTATION_HORIZONTAL) {
		newBox = gtk_hbox_new(FALSE, 0);
	} else {
		newBox = gtk_vbox_new(FALSE, 0);
	}
	gtk_widget_show (newBox);
	
	gtk_widget_reparent(st->panelValuesLabel, newBox);
	
	int chip, feature;
	for (chip=0; chip < st->sensorNumber; chip++) {
		for (feature=0; feature < FEATURES_PER_SENSOR; feature++) {
			if (st->sensorCheckBoxes[chip][feature] == TRUE) {
				t_barpanel *panel = (t_barpanel*) st->panels[chip][feature];
				gtk_widget_reparent (panel->databox, newBox);
			}
		}
	}
	
	gtk_widget_destroy (st->sensors);
	st->sensors = newBox;
	
	gtk_container_add (GTK_CONTAINER (st->eventbox), st->sensors);
	
	sensors_remove_graphical_panel(st);
    sensors_show_panel (st);
}

/*
static gboolean
global_update_function (gpointer data)
{
    #ifdef DEBUG
    g_printf ("global_update_function \n");
    #endif
    return sensors_show_panel (data);
} */


/* initialize box and label to pack them together */
static void
create_panel_widget (t_sensors * st) 
{
    #ifdef DEBUG
    g_printf ("create_panel_widget \n");
    #endif

    /* initialize a new vbox widget */
    if (st->orientation == GTK_ORIENTATION_HORIZONTAL)
        st->sensors = gtk_hbox_new (FALSE, 0);
    else
        st->sensors = gtk_vbox_new (FALSE, 0); 
    gtk_widget_show (st->sensors);
    
    /* initialize value label widget */
    st->panelValuesLabel = gtk_label_new(NULL);
    gtk_widget_show (st->panelValuesLabel);
    
    /* create 'valued' label */
    sensors_show_panel(st);
    
    /* add newly created label to box */
    gtk_box_pack_start(GTK_BOX (st->sensors), st->panelValuesLabel, FALSE, 
      FALSE, 0);
}



/* double-click improvement */
static gboolean
execute_command (GtkWidget *widget, GdkEventButton *event, gpointer data) 
{
    #ifdef DEBUG
    g_printf ("execute_command \n");
    #endif
   
   g_return_val_if_fail (data != NULL, FALSE);
   
   if (event->type == GDK_2BUTTON_PRESS) {

       t_sensors *st = (t_sensors *) data;
      
       g_return_val_if_fail ( st->execCommand, FALSE);
        
       GError *error;
        /* gboolean res = */
       xfce_exec (st->commandName, FALSE, FALSE, &error);
       
       return TRUE;
    }
    else
       return FALSE;
}


/* static gboolean
execCommandName_activate (GtkWidget *widget, GdkEventButton *event, gpointer data) 
{
    #ifdef DEBUG
    g_printf ("execCommandName_activate \n");
    #endif
    
    printf( "entry_text: %s \n", gtk_entry_get_text(GTK_ENTRY(widget)) );
    t_sensors *st = (t_sensors *) data;

    st->commandName = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));
    printf(" commandName: %s\n", st->commandName);

    return FALSE;
} */


static t_sensors *
sensors_new (XfcePanelPlugin *plugin)
{
    #ifdef DEBUG
    g_printf ("sensors_new \n");
    #endif

    t_sensors *st = g_new (t_sensors, 1);

    /* init xfce sensors stuff */    
    /* this is to be moved to read/write functions! */
    st->showTitle = TRUE;
    st->showLabels = TRUE;
    st->useBarUI = FALSE;
    st->barsCreated = FALSE;
    st->fontSize = "medium";
    st->fontSizeNumerical = 2;
    st->panelSize = 32; 
    st->sensorUpdateTime = 60;
    st->scale = CELSIUS;
    
    st->plugin = plugin;
    st->orientation = xfce_panel_plugin_get_orientation (plugin);
    
    /* double-click improvement */
    st->execCommand = TRUE;
    st->commandName = "xsensors";
    st->doubleClick_id = 0; 

    /* init libsensors stuff */
    FILE *filename = fopen("/etc/sensors.conf", "r");
    int sensorsInit = sensors_init(filename);
    if (sensorsInit != 0)
        g_printf(_("Error: Could not connect to sensors!"));
        
    st->sensorNumber = 0;
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
        while(nr1<FEATURES_PER_SENSOR) {
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
                    st->sensorRawValues [currentIndex] [nr1] = sensorFeature;
                        
   /* categorize sensor type */
   if ( strstr(st->sensorNames[currentIndex][nr1], "Temp")!=NULL 
      || strstr(st->sensorNames[currentIndex][nr1], "temp")!=NULL ) {
         st->sensor_types[currentIndex][nr1] = TEMPERATURE;
	 st->sensorMinValues[currentIndex][nr1] = 0;
	 st->sensorMaxValues[currentIndex][nr1] = 80;
   } else if ( strstr(st->sensorNames[currentIndex][nr1], "VCore")!=NULL 
      || strstr(st->sensorNames[currentIndex][nr1], "3V")!=NULL 
      || strstr(st->sensorNames[currentIndex][nr1], "5V")!=NULL 
      || strstr(st->sensorNames[currentIndex][nr1], "12V")!=NULL ) {
         st->sensor_types[currentIndex][nr1] = VOLTAGE;
	 st->sensorMinValues[currentIndex][nr1] = 2.8;
	 st->sensorMaxValues[currentIndex][nr1] = 12.2;
   } else if ( strstr(st->sensorNames[currentIndex][nr1], "Fan")!=NULL 
      || strstr(st->sensorNames[currentIndex][nr1], "fan")!=NULL ) {
         st->sensor_types[currentIndex][nr1] = SPEED;
	 st->sensorMinValues[currentIndex][nr1] = 1000;
	 st->sensorMaxValues[currentIndex][nr1] = 3500;
   } else {
         st->sensor_types[currentIndex][nr1] = OTHER;
	 st->sensorMinValues[currentIndex][nr1] = 0;
	 st->sensorMaxValues[currentIndex][nr1] = 7000;
   }                
                }  /* end if sensorFeature */
                else {
                    st->sensorValid [currentIndex] [nr1] = FALSE;
                }

            } /* end if sensorNames */
            
            st->sensorCheckBoxes [currentIndex] [nr1] = FALSE;
            
            nr1++;
        } /* end while nr1 */

        /* static problem if more sensors than supported! */
        if (currentIndex>=(SENSORS - 1)) break;

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
        st->sensorRawValues  [0] [0] = 0.0;
        st->sensorMinValues  [0] [0] = 0;
        st->sensorMaxValues  [0] [0] = 7000;
        st->sensorCheckBoxes [0] [0] = FALSE;
    }

    /* create eventbox to catch events on widget */
    st->eventbox = gtk_event_box_new ();
    gtk_widget_set_name (st->eventbox, "xfce_sensors");
    gtk_widget_show (st->eventbox);

    /* Add tooltip to show extended current sensors status */
    sensors_date_tooltip ((gpointer) st);

    /* fill panel widget with boxes, strings, values, ... */    
    create_panel_widget (st);
   
    /* finally add panel "sensors" to eventbox */
    gtk_container_add (GTK_CONTAINER (st->eventbox), st->sensors);

    /* update tooltip and widget data */
    /*
    st->timeout_id = g_timeout_add (st->sensorUpdateTime * 1000, 
                (GtkFunction) sensors_date_tooltip, (gpointer) st); */
                
    /* double-click improvement */
     st->doubleClick_id = g_signal_connect( G_OBJECT(st->eventbox), 
         "button-press-event", G_CALLBACK (execute_command), (gpointer) st);

    return st;
}

static void
sensors_free (XfcePanelPlugin *plugin, t_sensors *st)
{
    #ifdef DEBUG
    g_printf ("sensors_free \n");
    #endif
    /* stop association to libsensors */
    
    /* Why this??? Opening and closing afterwards really makes no sense! */
    
/*     FILE *filename = fopen("/etc/sensors.conf", "r");
    if (filename != NULL)
      {
        int closeResult = fclose(filename);
        if (closeResult!=0) 
           printf(_("A problem occured while trying to close the config file. \
Restart your computer ... err ... \
restart the sensor daemon only :-) \n"));
      } */

    g_return_if_fail (st != NULL);

    /* remove timeout functions */
    if (st->timeout_id)
    	g_source_remove (st->timeout_id);
        
    /* double-click improvement */
    g_source_remove (st->doubleClick_id);

    /* free structure */
    g_free (st);
}


static void
sensors_set_size (XfcePanelPlugin *plugin, int size, t_sensors *st)
{
    #ifdef DEBUG
    g_printf ("sensors_set_size \n");
    #endif
    st->panelSize = (gint) size;
    
    /* update the panel widget */
    sensors_show_panel ((gpointer) st);
}

static gint
getIdFromAddress (gint chip, gint addr, t_sensors* st) 
{
    #ifdef DEBUG
    g_printf ("getIdFromAddress \n");
    #endif
    gint id;
    for (id=0; id<st->sensorsCount[chip]; id++) {
        if (addr == st->sensorAddress[chip][id])
            return id;
    }
    
    return (gint) -1;
}

/* Write the configuration at exit */
static void
sensors_write_config (XfcePanelPlugin *plugin, t_sensors *st)
{
    #ifdef DEBUG
    g_printf ("sensors_write_config \n");
    #endif
    
    XfceRc *rc;
    char *file;

    if (!(file = xfce_panel_plugin_save_location (plugin, TRUE)))
        return;
    
    /* int res = */ unlink (file);
        
    rc = xfce_rc_simple_open (file, FALSE);
    g_free (file);

    if (!rc)
        return;
    
    xfce_rc_set_group (rc, "General");
    
    xfce_rc_write_bool_entry (rc, "Show_Title", st->showTitle);
    
    xfce_rc_write_bool_entry (rc, "Show_Labels", st->showLabels);
    
    xfce_rc_write_bool_entry (rc, "Use_Bar_UI", st->useBarUI);

    xfce_rc_write_int_entry (rc, "Scale", st->scale);

    xfce_rc_write_entry (rc, "Font_Size", st->fontSize);
    
    xfce_rc_write_int_entry (rc, "Font_Size_Numerical", 
                                st->fontSizeNumerical);
    
    xfce_rc_write_int_entry (rc, "Update_Interval", st->sensorUpdateTime);
    
    xfce_rc_write_bool_entry (rc, "Exec_Command", st->execCommand);

    xfce_rc_write_entry (rc, "Command_Name", st->commandName);
    
    int i;
    char chip[8];
    for (i=0; i<st->sensorNumber; i++) {
    
        g_snprintf (chip, 8, "Chip%d", i);
    
        xfce_rc_set_group (rc, chip);
    
        xfce_rc_write_entry (rc, "Name", st->sensorId[i]);
        
        /* number of sensors is still limited */
        xfce_rc_write_int_entry (rc, "Number", i);
        
        /* only save what was displayed to save time */
        int j;
        
        char feature[20];
        for (j=0; j<FEATURES_PER_SENSOR; j++) {
        
            if (st->sensorCheckBoxes[i][j] == TRUE) {
            
               g_snprintf (feature, 20, "%s_Feature%d", chip, j);
               
               xfce_rc_set_group (rc, feature);
                
               xfce_rc_write_int_entry (rc, "Id", getIdFromAddress(i, j, st));
                
               xfce_rc_write_int_entry (rc, "Address", j);
                
               xfce_rc_write_entry (rc, "Name", st->sensorNames[i][j]);
                
               xfce_rc_write_entry (rc, "Color", st->sensorColors[i][j]);
                
               xfce_rc_write_bool_entry (rc, "Show", st->sensorCheckBoxes[i][j]);

               xfce_rc_write_int_entry (rc, "Min", st->sensorMinValues[i][j]);

               xfce_rc_write_int_entry (rc, "Max", st->sensorMaxValues[i][j]);
            }
            
        } /* end for j */
        /* g_free (feature); */
        
    } /* end for i */
    /* g_free (chip); */
    
    xfce_rc_close (rc);
}


/* Read the configuration file at init */
static void
sensors_read_config (XfcePanelPlugin *plugin, t_sensors *st)
{
    #ifdef DEBUG
    g_printf ("sensors_read_config \n");
    #endif
    
    const char *value;
    char *file;
    XfceRc *rc;
    
    if (!(file = xfce_panel_plugin_lookup_rc_file (plugin)))
        return;
    
    rc = xfce_rc_simple_open (file, TRUE);
    g_free (file);

    if (!rc)
        return;
    
    if (xfce_rc_has_group (rc, "General") ) {

        xfce_rc_set_group (rc, "General");
        
        st->showTitle = xfce_rc_read_bool_entry (rc, "Show_Title", TRUE);

        st->showLabels = xfce_rc_read_bool_entry (rc, "Show_Labels", TRUE);

        st->useBarUI = xfce_rc_read_bool_entry (rc, "Use_Bar_UI", FALSE);

        st->scale = xfce_rc_read_int_entry (rc, "Scale", 0);

        if ((value = xfce_rc_read_entry (rc, "Font_Size", NULL)) && *value) {
            st->fontSize = g_strdup(value);
            /* g_free (value); */
        }
        
        st->fontSizeNumerical = xfce_rc_read_int_entry (rc,
                                                 "Font_Size_Numerical", 2);

        st->sensorUpdateTime = xfce_rc_read_int_entry (rc, "Update_Interval",
                                                  60);
        
        st->execCommand = xfce_rc_read_bool_entry (rc, "Exec_Command", TRUE);
        
        if ((value = xfce_rc_read_entry (rc, "Command_Name", NULL)) && *value) {
            st->commandName = g_strdup (value);
            /* g_free (value); */
        }
    }
    
    int i;
    char chip[8];
    
    for (i = 0; i<SENSORS; i++) {
        g_snprintf (chip, 8, "Chip%d", i);
        
        if (xfce_rc_has_group (rc, chip)) {
        
            xfce_rc_set_group (rc, chip);
        	
        	gchar* sensorName;
        	gint sensorNumber=0;
        	
        	if ((value = xfce_rc_read_entry (rc, "Name", NULL)) && *value) {
                sensorName = g_strdup (value);
                /* g_free (value); */
            }
            
            sensorNumber = (gint) xfce_rc_read_int_entry (rc, "Number", 0);
            
            /* assert that file does not contain more information 
              than does exist on system */ 
              /* ??? At least, it works. */
            g_return_if_fail (sensorNumber < st->sensorNumber);
            
            /* now featuring enhanced string comparison */
            if ( strcmp(st->sensorId[sensorNumber], sensorName)==0 ) {
            
                int j;
                char feature[20];
                
                for (j=0; j<FEATURES_PER_SENSOR; j++) {
                    g_snprintf (feature, 20, "%s_Feature%d", chip, j);
                
                    if (xfce_rc_has_group (rc, feature)) {
                    
                        xfce_rc_set_group (rc, feature);
                
                    	gint id=0, address=0;
                	
                	    id = (gint) xfce_rc_read_int_entry (rc, "Id", 0);
                	    
                        address = (gint) xfce_rc_read_int_entry (rc, "Address", 0);
                        
                        /* assert correctly saved file */
                        g_return_if_fail 
                            (st->sensorAddress[sensorNumber][id] == address);
                    
                        if ((value = xfce_rc_read_entry (rc, "Name", NULL)) 
                                && *value) {
                            st->sensorNames[sensorNumber][address] = 
                            g_strdup (value);
                            /* g_free (value); */
                        }

                        if ((value = xfce_rc_read_entry (rc, "Color", NULL)) 
                                && *value) {
                            st->sensorColors[sensorNumber][address] = 
                            g_strdup (value);
                            /* g_free (value); */
                        }
                        
                        st->sensorCheckBoxes[sensorNumber][address] = 
                            xfce_rc_read_bool_entry (rc, "Show", FALSE);

                        st->sensorMinValues[sensorNumber][address] = 
                            xfce_rc_read_int_entry (rc, "Min", 0);

        		        st->sensorMaxValues[sensorNumber][address] = 
                            xfce_rc_read_int_entry (rc, "Max", 0);

                    } /* end if */
                    
                } /* end for */
                
            } /* end if */
            
            g_free (sensorName);
            
    	} /* end if */
    	
    } /* end for */
	
	/* g_free (value); /* issues WARNING:  Verarbeiten des Argumentes 1 von 
	   »g_free« streicht Qualifizierer von Zeiger-Zieltypen */
	
	xfce_rc_close (rc);
	
    st->timeout_id = g_timeout_add (st->sensorUpdateTime * 1000, 
                (GtkFunction) sensors_show_panel, (gpointer) st);

    if (!st->execCommand) {
        g_signal_handler_block ( G_OBJECT(st->eventbox), st->doubleClick_id );
    }
                
    /* Try to resize the sensors to fit the user settings.
       Do also modify the tooltip text. */
    sensors_show_panel ((gpointer) st);
}


static void
show_title_toggled (GtkWidget *widget, SensorsDialog *sd)
{
    #ifdef DEBUG
    g_printf ("show_title_toggled \n");
    #endif
    
    sd->sensors->showTitle = gtk_toggle_button_get_active
        ( GTK_TOGGLE_BUTTON(widget) );
    if (sd->sensors->useBarUI == TRUE) {
	    sensors_remove_graphical_panel(sd->sensors);
    }
    sensors_show_panel ((gpointer) sd->sensors);
}


static void
show_labels_toggled (GtkWidget *widget, SensorsDialog *sd)
{
    #ifdef DEBUG
    g_printf ("show_labels_toggled \n");
    #endif
    sd->sensors->showLabels = gtk_toggle_button_get_active
        ( GTK_TOGGLE_BUTTON(widget) );
    if (sd->sensors->useBarUI == TRUE) {
	    sensors_remove_graphical_panel(sd->sensors);
    }
    sensors_show_panel ((gpointer) sd->sensors);
}


static void
ui_style_changed (GtkWidget *widget, SensorsDialog *sd)
{
    #ifdef DEBUG
    g_printf ("ui_style_changed \n");
    #endif
    if (sd->sensors->useBarUI == TRUE) {
	    sensors_remove_graphical_panel(sd->sensors);
    }
    sd->sensors->useBarUI = gtk_toggle_button_get_active
        ( GTK_TOGGLE_BUTTON(widget) );
    gtk_widget_set_sensitive(sd->labelsBox, sd->sensors->useBarUI);
    gtk_widget_set_sensitive(sd->fontBox, !sd->sensors->useBarUI);
    
    sensors_show_panel ((gpointer) sd->sensors);
}


static void 
sensor_entry_changed (GtkWidget *widget, SensorsDialog *sd)
{
    #ifdef DEBUG
    g_printf ("sensor_entry_changed \n");
    #endif

    gint gtk_combo_box_active = 
        gtk_combo_box_get_active(GTK_COMBO_BOX (widget));

    /* widget should be sd->myComboBox */
    gtk_label_set_label (GTK_LABEL(sd->mySensorLabel), 
            (const gchar*) sensors_get_adapter_name
                (sd->sensors->chipName[gtk_combo_box_active]->bus) );
    
    gtk_tree_view_set_model ( 
        GTK_TREE_VIEW (sd->myTreeView), 
        GTK_TREE_MODEL (sd->myListStore [ gtk_combo_box_active ]) );
}


static void
gtk_font_size_change (GtkWidget *widget, SensorsDialog *sd)
{
    #ifdef DEBUG
    g_printf ("gtk_font_size_change \n");
    #endif

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
    sensors_show_panel ((gpointer) sd->sensors);
}


static void
gtk_temperature_unit_change (GtkWidget *widget, SensorsDialog *sd)
{
    #ifdef DEBUG
    g_printf ("gtk_temperature_unit_change ");
    #endif
    
    /* toggle celsius-fahrenheit by use of mathematics ;) */
    sd->sensors->scale = 1 - sd->sensors->scale;

    /* refresh the panel content */
    sensors_show_panel((gpointer) sd->sensors);
}


static void
adjustment_value_changed (GtkWidget *widget, SensorsDialog* sd)
{
    #ifdef DEBUG
    g_printf ("adjustment_value_changed ");
    #endif
    
    sd->sensors->sensorUpdateTime = 
        (gint) gtk_adjustment_get_value ( GTK_ADJUSTMENT (widget) );
    
    /* stop the timeout functions ... */
    g_source_remove (sd->sensors->timeout_id);
    /* ... and start them again */
    sd->sensors->timeout_id  = g_timeout_add ( 
        sd->sensors->sensorUpdateTime * 1000, 
        (GtkFunction) sensors_show_panel, (gpointer) sd->sensors);
}

/* double-click improvement */
static void
execCommand_toggled (GtkWidget *widget, SensorsDialog* sd)
{
    #ifdef DEBUG
    g_printf ("execCommand_toggled \n");
    #endif
   sd->sensors->execCommand = 
         gtk_toggle_button_get_active ( GTK_TOGGLE_BUTTON (widget) );

   if ( sd->sensors->execCommand )
      g_signal_handler_unblock (sd->sensors->eventbox, 
            sd->sensors->doubleClick_id);
   else 
      g_signal_handler_block (sd->sensors->eventbox, 
            sd->sensors->doubleClick_id );
}


static void
minimum_changed (GtkCellRendererText *cellrenderertext, gchar *path_str,
                 gchar *new_value, SensorsDialog *sd)
{
    #ifdef DEBUG
    g_printf ("minimum_changed \n");
    #endif
	gint value = atol(new_value);

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
	gtk_tree_store_set (GTK_TREE_STORE (model), &iter, 4, value, -1);
	int help = sd->sensors->sensorAddress  [gtk_combo_box_active]
		[atoi(path_str)];
	sd->sensors->sensorMinValues[gtk_combo_box_active][help] = value;
        
	/* clean up */
	gtk_tree_path_free (path);

	if (sd->sensors->useBarUI == TRUE) {
		sensors_remove_graphical_panel (sd->sensors);
	}

	/* update panel */
	sensors_show_panel ((gpointer) sd->sensors);
}


static void
maximum_changed (GtkCellRendererText *cellrenderertext, gchar *path_str,
			gchar *new_value, SensorsDialog *sd)
{
    #ifdef DEBUG
    g_printf ("maximum_changed \n");
    #endif
	gint value = atol(new_value);

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
	gtk_tree_store_set (GTK_TREE_STORE (model), &iter, 5, value, -1);
	int help = sd->sensors->sensorAddress  [gtk_combo_box_active]
		[atoi(path_str)];
	sd->sensors->sensorMaxValues[gtk_combo_box_active][help] = value;
        
	/* clean up */
	gtk_tree_path_free (path);

	if (sd->sensors->useBarUI == TRUE) {
		sensors_remove_graphical_panel (sd->sensors);
	}

	/* update panel */
	sensors_show_panel ((gpointer) sd->sensors);
}


static void
gtk_cell_color_edited (GtkCellRendererText *cellrenderertext, gchar *path_str, 
                       gchar *new_color, SensorsDialog *sd)
{
    #ifdef DEBUG
    g_printf ("gtk_cell_color_edited \n");
    #endif

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
    #ifdef DEBUG
    g_printf ("gtk_cell_text_edited \n");
    #endif

    if (sd->sensors->useBarUI == TRUE) {
	    sensors_remove_graphical_panel(sd->sensors);
    }
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
        g_strdup (new_text);
                        
    /* clean up */
    gtk_tree_path_free (path);
    
    /* update panel */
    sensors_show_panel ((gpointer) sd->sensors);
}


static void
gtk_cell_toggle (GtkCellRendererToggle *cell, gchar *path_str, 
                  SensorsDialog *sd)
{
    #ifdef DEBUG
    g_printf ("gtk_cell_toggle \n");
    #endif

    if (sd->sensors->useBarUI == TRUE) {
	    sensors_remove_graphical_panel(sd->sensors);
    }
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
    sensors_show_panel ((gpointer) sd->sensors);
}


static void
init_widgets (SensorsDialog *sd)
{
    #ifdef DEBUG
    g_printf ("init_widgets \n");
    #endif
    
    int i=0;
    while( i < sd->sensors->sensorNumber ) {
        sd->myListStore[i] = gtk_tree_store_new (6, G_TYPE_STRING, 
          			G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_STRING,
				G_TYPE_INT, G_TYPE_INT);
        i++;
    }
    
    GtkTreeIter iter;
    
    i=0;
    while( i < sd->sensors->sensorNumber ) {
        
        gtk_combo_box_append_text ( GTK_COMBO_BOX(sd->myComboBox), 
                                    sd->sensors->sensorId[i] );
        
        int nr1=0;

        while( nr1 < FEATURES_PER_SENSOR ) {
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
		sd->sensors->sensorRawValues[i][nr1] = sensorFeature;
                gtk_tree_store_append ( GTK_TREE_STORE (sd->myListStore[i]), 
                                        &iter, NULL);
                gtk_tree_store_set ( GTK_TREE_STORE (sd->myListStore[i]),
                                     &iter,
                                     0, sd->sensors->sensorNames[i][nr1] ,
                                     1, sd->sensors->sensorValues[i][nr1] ,
                                     2, sd->sensors->sensorCheckBoxes[i][nr1],
                                     3, sd->sensors->sensorColors[i][nr1],
                                     4, sd->sensors->sensorMinValues[i][nr1],
                                     5, sd->sensors->sensorMaxValues[i][nr1],
                                     -1);
            } /* end if sensors-valid */

            nr1++;
        } /* end while nr1 */

        i++;
    } /* end while i < sensorNumber */
    
    if(sd->sensors->sensorNumber == 0) {
        gtk_combo_box_append_text ( GTK_COMBO_BOX(sd->myComboBox), 
                                sd->sensors->sensorId[0] );
        sd->myListStore[0] = gtk_tree_store_new (6, G_TYPE_STRING, 
                            G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_STRING,
			    G_TYPE_INT, G_TYPE_INT);
        sd->sensors->sensorValues[0][0] = 
                            g_strdup_printf("%+5.2f", 0.0);
        sd->sensors->sensorRawValues[0][0] = 0.0;
        gtk_tree_store_append ( GTK_TREE_STORE (sd->myListStore[0]), 
                            &iter, NULL);
        gtk_tree_store_set ( GTK_TREE_STORE (sd->myListStore[0]),
                            &iter,
                            0, sd->sensors->sensorNames[0][0] ,
                            1, sd->sensors->sensorValues[0][0] ,
                            2, sd->sensors->sensorCheckBoxes[0][0],
                            3, sd->sensors->sensorColors[0][0],
                            4, sd->sensors->sensorMinValues[0][0],
                            5, sd->sensors->sensorMaxValues[0][0],
                            -1);
    }
    
}


static void
add_ui_style_box (GtkWidget * vbox, SensorsDialog * sd)
{
    #ifdef DEBUG
    g_printf ("add_ui_style_box \n");
    #endif

    GtkWidget *hbox, *checkButton;

    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);

    GtkWidget *label = gtk_label_new(_("UI style:"));
    GtkWidget *radioText = gtk_radio_button_new_with_label(NULL, _("text"));
    GtkWidget *radioBars = gtk_radio_button_new_with_label(
	       gtk_radio_button_group(GTK_RADIO_BUTTON(radioText)), _("graphical"));
    
    gtk_widget_show(radioText);
    gtk_widget_show(radioBars);
    gtk_widget_show(label);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radioText),
					sd->sensors->useBarUI == FALSE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radioBars),
					sd->sensors->useBarUI == TRUE);

    gtk_box_pack_start(GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX (hbox), radioText, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX (hbox), radioBars, FALSE, FALSE, 0);

    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

    g_signal_connect (G_OBJECT (radioBars), "toggled", 
                      G_CALLBACK (ui_style_changed), sd );
}


static void
add_labels_box (GtkWidget * vbox, SensorsDialog * sd)
{
    #ifdef DEBUG
    g_printf ("add_labels_box \n");
    #endif

    GtkWidget *hbox, *checkButton;

    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    sd->labelsBox = hbox;
    gtk_widget_set_sensitive(hbox, sd->sensors->useBarUI);

    checkButton = gtk_check_button_new_with_label (
         _("Show labels in graphical UI"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(checkButton), 
                                  sd->sensors->showLabels);
    gtk_widget_show (checkButton);
    
    gtk_box_pack_start (GTK_BOX (hbox), checkButton, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);
        
    g_signal_connect (G_OBJECT (checkButton), "toggled", 
                      G_CALLBACK (show_labels_toggled), sd );
}

	
static void
add_title_box (GtkWidget * vbox, SensorsDialog * sd)
{
    #ifdef DEBUG
    g_printf ("add_title_box \n");
    #endif

    GtkWidget *hbox, *checkButton;

    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);

    checkButton = gtk_check_button_new_with_label (_("Show title"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(checkButton), 
                                  sd->sensors->showTitle);
    gtk_widget_show (checkButton);
    
    gtk_box_pack_start (GTK_BOX (hbox), checkButton, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);
        
    g_signal_connect (G_OBJECT (checkButton), "toggled", 
                      G_CALLBACK (show_title_toggled), sd );
}


static void
add_type_box (GtkWidget * vbox, SensorsDialog * sd)
{
    #ifdef DEBUG
    g_printf ("add_type_box \n");
    #endif

    GtkWidget *hbox, *label;

    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new (_("Sensors type:"));
    gtk_widget_show (label);
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    
    gtk_widget_show (sd->myComboBox);
    gtk_box_pack_start (GTK_BOX (hbox), sd->myComboBox, FALSE, FALSE, 0);
    
    
    gint gtk_combo_box_active = 
        gtk_combo_box_get_active(GTK_COMBO_BOX(sd->myComboBox));
    
    if (sd->sensors->sensorNumber >0)
        sd->mySensorLabel = gtk_label_new 
            ( sensors_get_adapter_name
                ( sd->sensors->chipName[gtk_combo_box_active ]->bus) );
    else
        sd->mySensorLabel = 
            gtk_label_new ( sd->sensors->sensorId [gtk_combo_box_active ] );
    
    gtk_widget_show (sd->mySensorLabel);
    gtk_box_pack_start (GTK_BOX (hbox), sd->mySensorLabel, FALSE, FALSE, 0);
    
    g_signal_connect (G_OBJECT (sd->myComboBox), "changed", 
                      G_CALLBACK (sensor_entry_changed), sd );
}


static void
add_sensor_settings_box ( GtkWidget * vbox, SensorsDialog * sd)
{
    #ifdef DEBUG
    g_printf ("add_sensor_settings_box \n");
    #endif

    GtkTreeViewColumn *aTreeViewColumn;
    GtkCellRenderer *myCellRendererText;
    
    gint gtk_combo_box_active = 
        gtk_combo_box_get_active(GTK_COMBO_BOX(sd->myComboBox));
        
    sd->myTreeView = gtk_tree_view_new_with_model
        ( GTK_TREE_MODEL ( sd->myListStore[ gtk_combo_box_active ] ) );

    myCellRendererText = gtk_cell_renderer_text_new ();
    g_object_set ( (gpointer*) myCellRendererText, "editable", TRUE, NULL );

    aTreeViewColumn = gtk_tree_view_column_new_with_attributes (_("Name"), 
                        myCellRendererText, "text", 0, NULL);
    g_signal_connect    (G_OBJECT (myCellRendererText), "edited", 
                        G_CALLBACK (gtk_cell_text_edited), sd);
    gtk_tree_view_column_set_expand (aTreeViewColumn, TRUE);
    gtk_tree_view_append_column (GTK_TREE_VIEW (sd->myTreeView), 
                        GTK_TREE_VIEW_COLUMN (aTreeViewColumn));
    
    myCellRendererText = gtk_cell_renderer_text_new ();
    aTreeViewColumn = gtk_tree_view_column_new_with_attributes (_("Value"), 
                        myCellRendererText, "text", 1, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (sd->myTreeView), 
                        GTK_TREE_VIEW_COLUMN (aTreeViewColumn));
    
    GtkCellRenderer *myCellRendererToggle = gtk_cell_renderer_toggle_new();
    aTreeViewColumn = gtk_tree_view_column_new_with_attributes (_("Show"), 
                        myCellRendererToggle, "active", 2, NULL);
    g_signal_connect    (G_OBJECT (myCellRendererToggle), "toggled", 
                        G_CALLBACK (gtk_cell_toggle), sd );
    gtk_tree_view_append_column (GTK_TREE_VIEW (sd->myTreeView), 
                        GTK_TREE_VIEW_COLUMN (aTreeViewColumn));
    
    myCellRendererText = gtk_cell_renderer_text_new ();
    g_object_set ( (gpointer*) myCellRendererText, "editable", TRUE, NULL );
    aTreeViewColumn = gtk_tree_view_column_new_with_attributes (_("Color"), 
                        myCellRendererText, "text", 3, NULL);
    g_signal_connect    (G_OBJECT (myCellRendererText), "edited", 
                        G_CALLBACK (gtk_cell_color_edited), sd);
    gtk_tree_view_append_column (GTK_TREE_VIEW (sd->myTreeView), 
                        GTK_TREE_VIEW_COLUMN (aTreeViewColumn));

    myCellRendererText = gtk_cell_renderer_text_new ();
    g_object_set ( (gpointer*) myCellRendererText, "editable", TRUE, NULL );
    aTreeViewColumn = gtk_tree_view_column_new_with_attributes
		    		(_("Min"), myCellRendererText, "text", 4, NULL);
    g_signal_connect(G_OBJECT(myCellRendererText), "edited",
		    			G_CALLBACK(minimum_changed), sd);
    gtk_tree_view_append_column(GTK_TREE_VIEW(sd->myTreeView),
		    			GTK_TREE_VIEW_COLUMN(aTreeViewColumn));

    myCellRendererText = gtk_cell_renderer_text_new ();
    g_object_set ( (gpointer*) myCellRendererText, "editable", TRUE, NULL );
    aTreeViewColumn = gtk_tree_view_column_new_with_attributes
		    		(_("Max"), myCellRendererText, "text", 5, NULL);
    g_signal_connect(G_OBJECT(myCellRendererText), "edited",
		    			G_CALLBACK(maximum_changed), sd);
    gtk_tree_view_append_column(GTK_TREE_VIEW(sd->myTreeView),
		    			GTK_TREE_VIEW_COLUMN(aTreeViewColumn));

    GtkWidget *myScrolledWindow = gtk_scrolled_window_new (NULL, NULL);
        gtk_scrolled_window_set_policy (
            GTK_SCROLLED_WINDOW (myScrolledWindow), GTK_POLICY_AUTOMATIC, 
            GTK_POLICY_AUTOMATIC);
        gtk_container_set_border_width (GTK_CONTAINER (myScrolledWindow), 0);
        gtk_scrolled_window_add_with_viewport (
            GTK_SCROLLED_WINDOW (myScrolledWindow), sd->myTreeView);

    gtk_box_pack_start (GTK_BOX (vbox), myScrolledWindow, TRUE, TRUE, 0);

    gtk_widget_show (sd->myTreeView);
    gtk_widget_show (myScrolledWindow);
}


static void
add_font_size_box (GtkWidget * vbox, SensorsDialog * sd)
{
    #ifdef DEBUG
    g_printf ("add_font_size_box \n");
    #endif

    GtkWidget *myFontLabel = gtk_label_new (_("Font size:"));
    GtkWidget *myFontBox = gtk_hbox_new(FALSE, BORDER);
    GtkWidget *myFontSizeComboBox = gtk_combo_box_new_text();

    sd->fontBox = myFontBox;
    gtk_widget_set_sensitive(myFontBox, !sd->sensors->useBarUI);

    gtk_combo_box_append_text (GTK_COMBO_BOX(myFontSizeComboBox), _("x-small"));
    gtk_combo_box_append_text (GTK_COMBO_BOX(myFontSizeComboBox), _("small")  );
    gtk_combo_box_append_text (GTK_COMBO_BOX(myFontSizeComboBox), _("medium") );
    gtk_combo_box_append_text (GTK_COMBO_BOX(myFontSizeComboBox), _("large")  );
    gtk_combo_box_append_text (GTK_COMBO_BOX(myFontSizeComboBox), _("x-large"));
    gtk_combo_box_set_active  (GTK_COMBO_BOX(myFontSizeComboBox), 
        sd->sensors->fontSizeNumerical);

    gtk_box_pack_start (GTK_BOX (myFontBox), myFontLabel, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (myFontBox), myFontSizeComboBox, FALSE, FALSE,
        0);    
    gtk_box_pack_start (GTK_BOX (vbox), myFontBox, FALSE, FALSE, 0);
    
    gtk_widget_show (myFontLabel);
    gtk_widget_show (myFontSizeComboBox);
    gtk_widget_show (myFontBox);
    
    g_signal_connect   (G_OBJECT (myFontSizeComboBox), "changed", 
                        G_CALLBACK (gtk_font_size_change), sd );
}

static void
add_temperature_unit_box (GtkWidget *vbox, SensorsDialog *sd)
{    
    #ifdef DEBUG
    g_printf ("add_temperature_unit_box \n");
    #endif
    GtkWidget *hbox;

    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);

    GtkWidget *label = gtk_label_new(_("Temperature scale:"));
    GtkWidget *radioCelsius = gtk_radio_button_new_with_label (NULL, 
                                                              _("Celsius"));
    GtkWidget *radioFahrenheit = gtk_radio_button_new_with_label(
      gtk_radio_button_get_group(GTK_RADIO_BUTTON(radioCelsius)), _("Fahrenheit"));
    
    gtk_widget_show(radioCelsius);
    gtk_widget_show(radioFahrenheit);
    gtk_widget_show(label);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radioCelsius),
					sd->sensors->scale == CELSIUS);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radioFahrenheit),
					sd->sensors->scale == FAHRENHEIT);

    gtk_box_pack_start(GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX (hbox), radioCelsius, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX (hbox), radioFahrenheit, FALSE, FALSE, 0);

    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

    g_signal_connect (G_OBJECT (radioCelsius), "toggled", 
                      G_CALLBACK (gtk_temperature_unit_change), sd );
}


static void
add_update_time_box (GtkWidget * vbox, SensorsDialog * sd)
{
    #ifdef DEBUG
    g_printf ("add_update_time_box \n");
    #endif
    
    GtkWidget *spinner, *myLabel, *myBox;
    GtkAdjustment *spinner_adj;

    spinner_adj = (GtkAdjustment *) gtk_adjustment_new (
/* TODO: restore original */
        sd->sensors->sensorUpdateTime, 1.0, 990.0, 1.0, 60.0, 60.0);
   
    /* creates the spinner, with no decimal places */
    spinner = gtk_spin_button_new (spinner_adj, 10.0, 0);

    myLabel = gtk_label_new (_("Update interval (seconds):"));
    myBox = gtk_hbox_new(FALSE, BORDER);

    gtk_box_pack_start (GTK_BOX (myBox), myLabel, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (myBox), spinner, FALSE, FALSE, 0);    
    gtk_box_pack_start (GTK_BOX (vbox), myBox, FALSE, FALSE, 0);
    
    gtk_widget_show (myLabel);
    gtk_widget_show (spinner);
    gtk_widget_show (myBox);
    
    g_signal_connect   (G_OBJECT (spinner_adj), "value_changed",
                        G_CALLBACK (adjustment_value_changed), sd );
}

/* double-click improvement */  
static void
add_command_box (GtkWidget * vbox,  SensorsDialog * sd)
{
    #ifdef DEBUG
    g_printf ("add_command_box \n");
    #endif
    
    GtkWidget *myBox;
    myBox = gtk_hbox_new(FALSE, BORDER);

    sd->myExecCommandCheckBox = gtk_check_button_new_with_label 
        (_("Execute on double click:"));
    gtk_toggle_button_set_active 
        ( GTK_TOGGLE_BUTTON (sd->myExecCommandCheckBox), 
        sd->sensors->execCommand );

    sd->myCommandNameEntry = gtk_entry_new ();
    gtk_widget_set_size_request (sd->myCommandNameEntry, 160, 25);
    
    
    gtk_entry_set_text( GTK_ENTRY(sd->myCommandNameEntry), 
        sd->sensors->commandName ); 

    gtk_box_pack_start (GTK_BOX (myBox), sd->myExecCommandCheckBox, FALSE,
                        FALSE, 0);
    gtk_box_pack_start (GTK_BOX (myBox), sd->myCommandNameEntry, FALSE, 
                        FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), myBox, FALSE, FALSE, 0);

    gtk_widget_show (sd->myExecCommandCheckBox);
    gtk_widget_show (sd->myCommandNameEntry);
    gtk_widget_show (myBox);

    g_signal_connect  (G_OBJECT (sd->myExecCommandCheckBox), "toggled",
                    G_CALLBACK (execCommand_toggled), sd );
                    
    /* g_signal_connect  (G_OBJECT (sd->myCommandNameEntry), "focus-out-event",
                    G_CALLBACK (execCommandName_activate), sd ); */

} 


static void 
add_view_frame (GtkWidget * vbox, SensorsDialog * sd)
{
    #ifdef DEBUG
    g_printf ("add_view_frame \n");
    #endif
    GtkWidget *_vbox = gtk_vbox_new (FALSE, 4);
    gtk_container_set_border_width (GTK_CONTAINER(_vbox), 4);
    gtk_widget_show (_vbox);
    
    GtkWidget *_label = gtk_label_new("");
    gtk_label_set_markup(GTK_LABEL(_label),  _("<b>View</b>"));
    gtk_widget_show (_label);
    
    GtkWidget *_frame = gtk_frame_new ("");
    gtk_frame_set_label_widget (GTK_FRAME(_frame), _label);
    gtk_widget_show (_frame);
    gtk_container_add (GTK_CONTAINER (_frame), _vbox);
    gtk_box_pack_start (GTK_BOX (vbox), _frame, FALSE, FALSE, 4);

    add_title_box (_vbox, sd);

    add_ui_style_box (_vbox, sd);

    add_labels_box (_vbox, sd);

    add_font_size_box (_vbox, sd);
}


static void 
add_sensors_frame (GtkWidget * vbox, SensorsDialog * sd)
{
    #ifdef DEBUG
    g_printf ("add_sensors_frame \n");
    #endif
    GtkWidget *_vbox = gtk_vbox_new (FALSE, 4);
    gtk_container_set_border_width (GTK_CONTAINER(_vbox), 4);
    gtk_widget_show (_vbox);
    
    GtkWidget *_label = gtk_label_new("");
    gtk_label_set_markup(GTK_LABEL(_label),  _("<b>Sensors</b>"));
    gtk_widget_show (_label);
    
    GtkWidget *_frame = gtk_frame_new ("");
    gtk_frame_set_label_widget (GTK_FRAME(_frame), _label);
    gtk_widget_show (_frame);
    gtk_container_add (GTK_CONTAINER (_frame), _vbox);
    gtk_box_pack_start (GTK_BOX (vbox), _frame, TRUE, TRUE, 4);
    
    add_type_box (_vbox, sd);

    add_sensor_settings_box (_vbox, sd);
    
    add_temperature_unit_box (_vbox, sd);
}


static void 
add_miscellaneous_frame (GtkWidget * vbox, SensorsDialog * sd)
{
    #ifdef DEBUG
    g_printf ("add_miscellaneous_frame \n");
    #endif
    GtkWidget *_vbox = gtk_vbox_new (FALSE, 4);
    gtk_container_set_border_width (GTK_CONTAINER(_vbox), 4);
    gtk_widget_show (_vbox);
    
    GtkWidget *_label = gtk_label_new("");
    gtk_label_set_markup(GTK_LABEL(_label),  _("<b>Miscellaneous</b>"));
    gtk_widget_show (_label);
    
    GtkWidget *_frame = gtk_frame_new ("");
    gtk_frame_set_label_widget (GTK_FRAME(_frame), _label);
    gtk_widget_show (_frame);
    gtk_container_add (GTK_CONTAINER (_frame), _vbox);
    gtk_box_pack_start (GTK_BOX (vbox), _frame, FALSE, FALSE, 4);
   
    add_update_time_box (_vbox, sd);
   
    add_command_box (_vbox, sd);
}


static void 
on_optionsDialog_response (GtkWidget *dlg, int response, SensorsDialog *sd)
{    
    #ifdef DEBUG
    g_printf ("on_optionsDialog_response \n");
    #endif
    if (response==GTK_RESPONSE_OK) {
        /* FIXME: save most of the content in this function,
           remove those toggle functions where possible. NYI */
    	/* sensors_apply_options (sd); */
    	sd->sensors->commandName = 
            g_strdup ( gtk_entry_get_text(GTK_ENTRY(sd->myCommandNameEntry)) );
            
    	sensors_write_config (sd->sensors->plugin, sd->sensors);
    }

    gtk_widget_destroy (sd->dialog);
    xfce_panel_plugin_unblock_menu (sd->sensors->plugin);
}


/* create sensor options box */
static void
sensors_create_options (XfcePanelPlugin *plugin, t_sensors *st)
{
    #ifdef DEBUG
    g_printf ("sensors_create_options \n");
    #endif
    
    xfce_panel_plugin_block_menu (plugin);
    
    GtkWidget *dlg, *header;
    dlg = gtk_dialog_new_with_buttons (_("Edit Properties"), 
                GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (plugin))),
                GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_NO_SEPARATOR,
                GTK_STOCK_CLOSE, GTK_RESPONSE_OK, NULL);
                
    gtk_container_set_border_width (GTK_CONTAINER (dlg), 2);
   
    header = xfce_create_header (NULL, _("Sensors Plugin"));
    gtk_widget_set_size_request (GTK_BIN (header)->child, -1, 32);
    gtk_container_set_border_width (GTK_CONTAINER (header), BORDER - 2);
    gtk_widget_show (header);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), header, 
                        FALSE, TRUE, 0);

    GtkWidget *vbox = GTK_DIALOG (dlg)->vbox;
    SensorsDialog *sd;

    sd = g_new0 (SensorsDialog, 1);

    sd->sensors = st;
    sd->dialog = dlg;
    
    sd->myComboBox = gtk_combo_box_new_text();
        
    init_widgets (sd);
    
    gtk_combo_box_set_active (GTK_COMBO_BOX(sd->myComboBox), 0);
    
    add_view_frame (vbox, sd);
    add_sensors_frame (vbox, sd);
    add_miscellaneous_frame (vbox, sd);
    
    gtk_widget_set_size_request (vbox, 400, 500);

    g_signal_connect (dlg, "response",
            G_CALLBACK(on_optionsDialog_response), sd);
            
    gtk_widget_show (dlg);
}


/*  Sensors panel control
 *  ---------------------
 */
static t_sensors *
create_sensors_control (XfcePanelPlugin *plugin)
{
    #ifdef DEBUG
    g_printf ("create_sensors_control \n");
    #endif

    t_sensors *sensors = sensors_new (plugin);

    gtk_widget_show (sensors->eventbox);
   
    /* sensors_set_size (control, settings.size); */

    return sensors;
}


static void 
sensors_plugin_construct (XfcePanelPlugin *plugin)
{   
    #ifdef DEBUG
    g_printf ("sensors_plugin_construct \n");
    #endif
    xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

    t_sensors *sensors;

    sensors = create_sensors_control (plugin);

    sensors_read_config (plugin, sensors);
    
    g_signal_connect (plugin, "free-data", G_CALLBACK (sensors_free), sensors);
    
    g_signal_connect (plugin, "save", G_CALLBACK (sensors_write_config), 
                      sensors);
    
    xfce_panel_plugin_menu_show_configure (plugin);
    
    g_signal_connect (plugin, "configure-plugin", 
                      G_CALLBACK (sensors_create_options), sensors);
    
    g_signal_connect (plugin, "size-changed", G_CALLBACK (sensors_set_size), 
                         sensors);
    
    g_signal_connect (plugin, "orientation-changed", 
                      G_CALLBACK (sensors_set_orientation), sensors);
    
    gtk_container_add (GTK_CONTAINER(plugin), sensors->eventbox);

    xfce_panel_plugin_add_action_widget (plugin, sensors->eventbox);
}

XFCE_PANEL_PLUGIN_REGISTER_EXTERNAL (sensors_plugin_construct);
