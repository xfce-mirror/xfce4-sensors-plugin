#ifndef __SENSORS_INTERFACE_TYPES
#define __SENSORS_INTERFACE_TYPES

#include <gtk/gtk.h>

/**
 * compound widget displaying a levelbar and optional label
 */
typedef struct {
    /* the level bar */
    GtkWidget *progressbar;

    /* the label */
    GtkWidget *label;

    /* the surrounding box */
    GtkWidget *databox;
} t_labelledlevelbar;


/**
 * enumeration of the available visualization styles for the Xfce 4 Panel Sensors Plugin
 */
typedef enum {
  DISPLAY_TEXT = 1,
  DISPLAY_BARS,
  DISPLAY_TACHO
} e_displaystyles;

#endif
