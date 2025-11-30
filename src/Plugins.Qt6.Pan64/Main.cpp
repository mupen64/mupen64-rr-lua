/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Main.hpp"
#include <cstring>
#include <optional>

#include <portaudiocpp/DirectionSpecificStreamParameters.hxx>
#include <portaudiocpp/MemFunCallbackStream.hxx>
#include <portaudiocpp/PortAudioCpp.hxx>
#include <portaudiocpp/Stream.hxx>
#include <portaudiocpp/StreamParameters.hxx>

#include "Config.hpp"
#include "core_plugin.h"
#include "core_types.h"
#include "mupapi.h"

#include "SampleBuffer.hpp"

core_audio_info g_core_info;
std::optional<Pan64::SampleBuffer> g_sample_buffer;
std::optional<portaudio::MemFunCallbackStream<Pan64::SampleBuffer>> g_audio_stream;

EXPORT core_result CALL mup_init(const char *exe_dir, const core_plugin_extended_funcs *fwd_funcs)
{
    portaudio::System::initialize();
    return Res_Ok;
}

EXPORT void CALL mup_drop()
{
    portaudio::System::terminate();
}

EXPORT void CALL mup_get_info(core_plugin_info *info)
{
    info->ver = 0x0101;
    info->type = plugin_audio;
    strncpy(info->name, PLUGIN_NAME, sizeof(info->name));
}

EXPORT void CALL mup_rom_opened()
{
}

EXPORT void CALL mup_rom_closed()
{
    g_audio_stream->stop();
    g_audio_stream.reset();
    g_sample_buffer.reset();
}

EXPORT void CALL mupa_init(core_audio_info core_info)
{
    g_core_info = core_info;
    // init sample buffer
    g_sample_buffer.emplace(Pan64::Config{});

    // portaudio setup
    auto &pa = portaudio::System::instance();

    auto out_params = portaudio::DirectionSpecificStreamParameters{
        pa.defaultOutputDevice(),
        2,
        portaudio::FLOAT32,
        true,
        pa.defaultOutputDevice().defaultLowOutputLatency(),
        nullptr,
    };
    auto params = portaudio::StreamParameters {
        portaudio::DirectionSpecificStreamParameters::null(),
        out_params,
        44100,
        64,
        paClipOff,
    };

    g_audio_stream.emplace(params, *g_sample_buffer, &Pan64::SampleBuffer::pa_pull);
    g_audio_stream->start();
}

EXPORT void CALL mupa_ai_len_changed()
{
}

EXPORT uint32_t CALL mupa_ai_read_length()
{
    return 0;
}

EXPORT void mupa_process_a_list()
{
}

EXPORT void mupa_ai_update(int32_t wait)
{
}