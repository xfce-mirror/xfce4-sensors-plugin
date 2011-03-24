#ifndef __SENSORS_INTERFACE_TYPES
#define __SENSORS_INTERFACE_TYPES

#include <gtk/gtk.h>

/**
 * compound widget displaying a progressbar and optional label
 */
typedef struct {
    /* the progress bar */
    GtkWidget *progressbar;

    /* the label */
    GtkWidget *label;

    /* the surrounding box */
    GtkWidget *databox;
} t_barpanel;


typedef enum {
  DISPLAY_TEXT = 1,
  DISPLAY_BARS,
  DISPLAY_TACHO
} display_t;

#endif

