/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#pragma once

#include <QObject>
#include <QJSEngine>
#include <QJSValue>
#include <qqmlintegration.h>

#include <Core/Types.hpp>

class CoreResult : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
  public:
    /**
     * @brief Result returned by the core.
     *
     * Copied directly from `Core/Types.hpp`; allowing it to be used directly from Qt.
     */
    enum Value
    {
        // Generic
        // ==========================================

        // The operation completed successfully
        Res_Ok,

        // The operation was cancelled by the user
        Res_Cancelled,

        // VCR
        // ==========================================

        // The provided data has an invalid format
        VCR_InvalidFormat,
        // The provided file is inaccessible or does not exist
        VCR_BadFile,
        // The cheat data couldn't be written to disk
        VCR_CheatWriteFailed,
        // The controller configuration is invalid
        VCR_InvalidControllers,
        // The movie's savestate is missing or invalid
        VCR_InvalidSavestate,
        // The resulting frame is outside the bounds of the movie
        VCR_InvalidFrame,
        // There is no rom which matches this movie
        VCR_NoMatchingRom,
        // The VCR engine is idle, but must be active to complete this operation
        VCR_Idle,
        // The provided freeze buffer is not from the currently active movie
        VCR_NotFromThisMovie,
        // The movie's version is invalid
        VCR_InvalidVersion,
        // The movie's extended version is invalid
        VCR_InvalidExtendedVersion,
        // The operation requires a playback or recording task
        VCR_NeedsPlaybackOrRecording,
        // The operation requires a playback task
        VCR_NeedsPlayback,
        // The provided start type is invalid.
        VCR_InvalidStartType,
        // Another warp modify operation is already running
        VCR_WarpModifyAlreadyRunning,
        // Warp modifications can only be performed during recording
        VCR_WarpModifyNeedsRecordingTask,
        // The provided input buffer is empty
        VCR_WarpModifyEmptyInputBuffer,
        // Another seek operation is already running
        VCR_SeekAlreadyRunning,
        // The seek operation could not be initiated due to a savestate not being loaded successfully
        VCR_SeekSavestateLoadFailed,
        // The seek operation can't be initiated because the seek savestate interval is 0
        VCR_SeekSavestateIntervalZero,
        // The seek string is malformed
        VCR_SeekStringMalformed,

        // VR
        // ==========================================

        // Couldn't find a rom matching the provided movie
        VR_NoMatchingRom,
        // An error occured during plugin loading
        VR_PluginError,
        // The ROM or alternative rom source is invalid
        VR_RomInvalid,
        // The emulator isn't running yet
        VR_NotRunning,
        // Failed to open core streams
        VR_FileOpenFailed,

        // Savestates
        // ==========================================

        // The core isn't launched
        ST_CoreNotLaunched,
        // The savestate file wasn't found
        ST_NotFound,
        // The savestate couldn't be written to disk
        ST_FileWriteError,
        // Couldn't decompress the savestate
        ST_DecompressionError,
        // The event queue was too long
        ST_EventQueueTooLong,
        // The CPU registers contained invalid values
        ST_InvalidRegisters,

        // Plugins
        // ==========================================

        // The plugin library couldn't be loaded
        Pl_LoadLibraryFailed,
        // The plugin doesn't export a GetDllInfo function
        Pl_NoGetDllInfo,

        // Init
        // ==========================================

        // The core params are missing a critical component.
        IN_MissingComponent,
    };
    Q_ENUM(Value)

    static Value from_core(::core_result result) { return (Value)(int)result; }
    static ::core_result to_core(Value value) { return (::core_result)(int)value; }

    Q_INVOKABLE QJSValue message(Value value);
};

namespace CoreDialogType
{
Q_NAMESPACE
QML_ELEMENT

enum Value
{
    Error = fsvc_error,
    Warning = fsvc_warning,
    Information = fsvc_information,
};
Q_ENUM_NS(Value)

inline Value from_core(::core_dialog_type result)
{
    return (Value)(int)result;
}
inline ::core_dialog_type to_core(Value value)
{
    return (::core_dialog_type)(int)value;
}
} // namespace CoreDialogType
