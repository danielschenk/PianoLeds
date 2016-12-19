/**
 * @file
 * @copyright (c) Daniel Schenk, 2016
 * This file is part of MLC: MIDI Led strip Controller.
 * 
 * @date 16 Dec 2016
 * 
 * @brief Data model containing current configuration.
 */

#include "ConfigurationModel.h"

#define DEFAULT_CURRENTPRESET 7

/** The configuration model definition. */
typedef struct ConfigurationModel
{
    /** The currently active preset. */
    uint8_t currentPreset;
    
    /** Current preset subscribers. */
    CallbackList_t currentPresetSubscribers;
} ConfigurationModel_t;

/** The configuration model instance. */
static ConfigurationModel_t gs_Model;

void ConfigurationModel_Initialize()
{
    gs_Model.currentPreset = DEFAULT_CURRENTPRESET;
    CallbackList_Initialize(&gs_Model.currentPresetSubscribers);
}

uint8_t ConfigurationModel_GetCurrentPreset()
{
    return gs_Model.currentPreset;
}

void ConfigurationModel_SetCurrentPreset(uint8_t preset)
{
    gs_Model.currentPreset = preset;
    CallbackList_ProcessAll(&gs_Model.currentPresetSubscribers, &preset);
}

void ConfigurationModel_SubscribeCurrentPreset(Callback_t callback)
{
    CallbackList_Add(&gs_Model.currentPresetSubscribers, callback);
}

void ConfigurationModel_UnsubscribeCurrentPreset(Callback_t callback)
{
    CallbackList_Remove(&gs_Model.currentPresetSubscribers, callback);
}
