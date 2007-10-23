#ifndef XFCE4_SENSORS_LMSENSORS_H
#define XFCE4_SENSORS_LMSENSORS_H

#include <glib/garray.h>

/*
 *  Initialize libsensors by reading sensor config and other stuff
 * @Return: Number of found chip_features
 * @Param: Pointer to array of chips
 */
int initialize_libsensors (GPtrArray *chips);

/*
 * Refreshs an lmsensors chip's feature in sense of raw and formatted value

 * @Param chip_feature: Pointer to feature
 */
void refresh_lmsensors (gpointer chip_feature, gpointer data);

#endif /* XFCE4_SENSORS_LMSENSORS_H */
