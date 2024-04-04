/*
 * Copyright (c) 2017-2024 The Forge Interactive Inc.
 *
 * This is a part of Ephemeris.
 * This file(code) is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License
 * (https://creativecommons.org/licenses/by-nc/4.0/legalcode) Based on a work at https://github.com/ConfettiFX/The-Forge. You can not use
 * this code for commercial purposes.
 *
 */

#include "AppSettings.h"

#include "../../../The-Forge/Common_3/Application/Interfaces/IUI.h"

const char* gCameraScripts[CAMERA_SCRIPT_COUNTS] = { "MovingCloud.lua", "SurfaceToAtmosphere.lua", "FlyingSurface.lua", "TimeOfDay.lua",
                                                     "Sunset.lua" };

AppSettings gAppSettings;

UIComponent* pGuiSkyWindow = NULL;
UIComponent* pGuiCloudWindow = NULL;