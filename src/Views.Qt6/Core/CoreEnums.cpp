/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "CoreEnums.hpp"

#include <QQmlEngine>

QJSValue QmlCoreResult::message(Value value)
{
    using namespace std::literals;
    using namespace Qt::Literals;

    QLatin1StringView module;
    QLatin1StringView error;

    switch (value)
    {
    case Res_Ok:
    case Res_Cancelled:
    case VCR_InvalidControllers:
        return QJSValue::NullValue;
    case VCR_InvalidFormat:
        module = "VCR"_L1;
        error = "The provided data has an invalid format."_L1;
        break;
    case VCR_BadFile:
        module = "VCR"_L1;
        error = "The provided file is inaccessible or does not exist."_L1;
        break;
    case VCR_InvalidSavestate:
        module = "VCR"_L1;
        error = "The movie's savestate is missing or invalid."_L1;
        break;
    case VCR_InvalidFrame:
        module = "VCR"_L1;
        error = "The resulting frame is outside the bounds of the movie."_L1;
        break;
    case VCR_NoMatchingRom:
        module = "VCR"_L1;
        error = "There is no rom which matches this movie."_L1;
        break;
    case VCR_Idle:
        module = "VCR"_L1;
        error = "The VCR engine is idle, but must be active to complete this operation."_L1;
        break;
    case VCR_NotFromThisMovie:
        module = "VCR"_L1;
        error = "The provided freeze buffer is not from the currently active movie."_L1;
        break;
    case VCR_InvalidVersion:
        module = "VCR"_L1;
        error = "The movie's version is invalid."_L1;
        break;
    case VCR_InvalidExtendedVersion:
        module = "VCR"_L1;
        error = "The movie's extended version is invalid. It might be too new for this version of the emulator."_L1;
        break;
    case VCR_NeedsPlaybackOrRecording:
        module = "VCR"_L1;
        error = "The operation requires a playback or recording task."_L1;
        break;
    case VCR_NeedsPlayback:
        module = "VCR"_L1;
        error = "The operation requires a playback task."_L1;
        break;
    case VCR_InvalidStartType:
        module = "VCR"_L1;
        error = "The provided start type is invalid."_L1;
        break;
    case VCR_WarpModifyAlreadyRunning:
        module = "VCR"_L1;
        error = "Another warp modify operation is already running."_L1;
        break;
    case VCR_WarpModifyNeedsRecordingTask:
        module = "VCR"_L1;
        error = "Warp modifications can only be performed during recording."_L1;
        break;
    case VCR_WarpModifyEmptyInputBuffer:
        module = "VCR"_L1;
        error = "The provided input buffer is empty."_L1;
        break;
    case VCR_SeekSavestateLoadFailed:
        module = "VCR"_L1;
        error = "The seek operation could not be initiated due to a savestate not being loaded successfully."_L1;
        break;
    case VCR_SeekSavestateIntervalZero:
        module = "VCR"_L1;
        error = "The seek operation can't be initiated because the seek savestate interval is 0."_L1;
        break;
    case VCR_SeekStringMalformed:
        module = "VCR"_L1;
        error = "The seek string is malformed."_L1;
        break;
#pragma endregion
#pragma region VR
    case VR_NoMatchingRom:
        module = "Core"_L1;
        error = "The ROM couldn't be loaded. Couldn't find an appropriate ROM."_L1;
        break;
    case VR_PluginError:
        module = "Core"_L1;
        error = "One or more plugins couldn't be loaded. Verify that you have selected all four plugins."_L1;
        break;
    case VR_RomInvalid:
        module = "Core"_L1;
        error = "The ROM couldn't be loaded. Verify that the ROM is a valid N64 ROM."_L1;
        break;
    case VR_FileOpenFailed:
        module = "Core"_L1;
        error = "Failed to open streams to core files. Verify that Mupen is allowed disk access."_L1;
        break;
#pragma endregion
#pragma region Init
    case IN_MissingComponent:
        module = "Core"_L1;
        error = "The core params are missing a critical component."_L1;
        break;
#pragma endregion
    default:
        module = "Unknown"_L1;
        error = "Unknown error."_L1;
        break;
    }

    QJSValue result = qmlEngine(this)->newObject();
    result.setProperty(u"module"_s, QJSValue(module));
    result.setProperty(u"error"_s, QJSValue(error));
    return result;
}
