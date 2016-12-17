/**
 * @file
 * @copyright (c) Daniel Schenk, 2016
 * This file is part of MLC: MIDI Led strip Controller.
 * 
 * @date 16 Dec 2016
 * 
 * @brief Interface to the data model containing current configuration.
 */


#ifndef CONFIGURATIONMODEL_H_
#define CONFIGURATIONMODEL_H_

#include <stdint.h>

/**
 * Initialize the configuration model.
 */
void ConfigurationModel_Initialize();

/**
 * Get the current preset.
 * 
 * @return The current preset.
 */
uint8_t ConfigurationModel_GetCurrentPreset();

/**
 * Set the current preset.
 * 
 * @param preset    The new preset value.
 */
void ConfigurationModel_SetCurrentPreset(uint8_t preset);

/**
 * Subscribe for preset changes.
 * 
 * @param callback  The callback function to be called upon preset changes.
 */
void ConfigurationModel_SubscribeCurrentPreset(Callback_t callback);

/**
 * Unsubscribe from preset changes.
 * 
 * @param callback  The callback function to be removed from the list.
 */
void ConfigurationModel_UnsubscribeCurrentPreset(Callback_t callback);


#endif /* CONFIGURATIONMODEL_H_ */