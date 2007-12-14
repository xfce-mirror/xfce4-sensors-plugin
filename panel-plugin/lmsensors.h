#ifndef XFCE4_SENSORS_LMSENSORS_H
#define XFCE4_SENSORS_LMSENSORS_H

#include <glib/garray.h>

#include "types.h"

/*
 *  Initialize libsensors by reading sensor config and other stuff
 * @Param chips: Pointer to array of chips
 * @Return: Number of found chip_features
 */
int initialize_libsensors (GPtrArray *chips);

/*
 * Refreshs an lmsensors chip's feature in sense of raw and formatted value

 * @Param chip_feature: Pointer to feature
 */
void refresh_lmsensors (gpointer chip_feature, gpointer data);

/*
 * Get the value of subsensor/feature that is number in array of sensors.
 * @Param name: Structure of sensor description.
 * @Param number: number of feature to read the value from
 * @Param value: pointer where the double feature value is to be stored
 * @Return: 0 on success
 */
int sensors_get_feature_wrapper (const sensors_chip_name *name, int number,
                                 double *value);

/*
 * Free the additionally allocated structures in the sensors_chip_name
 * according to the version of libsensors.
 * @Param chip: Pointer to t_chip
 */
void free_lmsensors_chip (gpointer chip);


void
categorize_lmsensors_type (t_chipfeature *chipfeature,
                           sensors_feature *feature);

#endif /* XFCE4_SENSORS_LMSENSORS_H */
