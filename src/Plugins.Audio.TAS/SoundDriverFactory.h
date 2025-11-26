/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Azimer, Bobby Smiles).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#pragma once

#include "common.h"
#include "SoundDriverInterface.h"

class SoundDriverFactory {
private:
    typedef SoundDriverInterface* (*SoundDriverCreationFunction)();
    struct FactoryDriversStruct {
        SoundDriverType DriverType;
        SoundDriverCreationFunction CreateFunction;
        int Priority;
        char Description[100];
    };

private:
    SoundDriverFactory() {};
    static int FactoryNextSlot;
    static const int MAX_FACTORY_DRIVERS = 20;
    static FactoryDriversStruct FactoryDrivers[MAX_FACTORY_DRIVERS];

public:
    ~SoundDriverFactory() {};

    static SoundDriverInterface* CreateSoundDriver(SoundDriverType DriverID);
    static bool RegisterSoundDriver(SoundDriverType DriverType, SoundDriverCreationFunction CreateFunction, const char* Description, int Priority);
    static SoundDriverType DefaultDriver();
    static int EnumDrivers(SoundDriverType* drivers, int max_entries);
    static const char* GetDriverDescription(SoundDriverType driver);
    static bool DriverExists(SoundDriverType driver);
};
