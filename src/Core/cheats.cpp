/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "MiscHelpers.h"
#include <CommonPCH.h>
#include <Core.h>
#include <charconv>
#include <memory/memory.h>
#include <cheats.h>
#include <r4300/r4300.h>
#include <string>

static std::recursive_mutex cheats_mutex;
static std::vector<core_cheat> host_cheats;
static std::stack<std::vector<core_cheat>> cheat_stack;

bool core_cht_compile(std::string_view code, core_cheat &cheat)
{
    core_cheat compiled_cheat{};

    bool serial = false;
    size_t serial_count = 0;
    size_t serial_offset = 0;
    size_t serial_diff = 0;

    for (auto line : MiscHelpers::split_string(code, "\n"))
    {
        if (line[0] == '$' || line[0] == '-' || line.size() < 13)
        {
            g_core->log_info("[GS] Line skipped");
            continue;
        }

        auto opcode = line.substr(0, 2);

        uint32_t address = 0;
        uint32_t val = 0;

        {
            std::from_chars_result result;

            result = std::from_chars(&line[2], &line[8], address, 16);
            if (result.ec != std::errc{}) return false;

            if (line[8] == ' ')
                result = std::from_chars(&line[9], &line[13], val, 16);
            else
                result = std::from_chars(&line[10], &line[14], val, 16);

            if (result.ec != std::errc{}) return false;
        }

        if (serial)
        {
            g_core->log_info(std::format("[GS] Compiling {} serial byte writes...", serial_count));

            for (size_t i = 0; i < serial_count; ++i)
            {
                // Madghostek: warning, assumes that serial codes are writing bytes, which seems to match pj64
                // Madghostek: if not, change WB to WW
                compiled_cheat.instructions.emplace_back(std::make_tuple(false, [=] {
                    core_rdram_store<uint8_t>(rdramb, address + serial_offset * i, val + serial_diff * i);
                    return true;
                }));
            }
            serial = false;
            continue;
        }

        if (opcode == "80" || opcode == "A0")
        {
            // Write byte
            compiled_cheat.instructions.emplace_back(std::make_tuple(false, [=] {
                core_rdram_store<uint8_t>(rdramb, address, val & 0xFF);
                return true;
            }));
        }
        else if (opcode == "81" || opcode == "A1")
        {
            // Write word
            compiled_cheat.instructions.emplace_back(std::make_tuple(false, [=] {
                core_rdram_store<uint16_t>(rdramb, address, val);
                return true;
            }));
        }
        else if (opcode == "88")
        {
            // Write byte if GS button pressed
            compiled_cheat.instructions.emplace_back(std::make_tuple(false, [=] {
                if (g_ctx.vr_get_gs_button())
                {
                    core_rdram_store<uint8_t>(rdramb, address, val & 0xFF);
                }
                return true;
            }));
        }
        else if (opcode == "89")
        {
            // Write word if GS button pressed
            compiled_cheat.instructions.emplace_back(std::make_tuple(false, [=] {
                if (g_ctx.vr_get_gs_button())
                {
                    core_rdram_store<uint16_t>(rdramb, address, val);
                }
                return true;
            }));
        }
        else if (opcode == "D0")
        {
            // Byte equality comparison
            compiled_cheat.instructions.emplace_back(
                std::make_tuple(true, [=] { return core_rdram_load<uint8_t>(rdramb, address) == (val & 0xFF); }));
        }
        else if (opcode == "D1")
        {
            // Word equality comparison
            compiled_cheat.instructions.emplace_back(
                std::make_tuple(true, [=] { return core_rdram_load<uint16_t>(rdramb, address) == val; }));
        }
        else if (opcode == "D2")
        {
            // Byte inequality comparison
            compiled_cheat.instructions.emplace_back(
                std::make_tuple(true, [=] { return core_rdram_load<uint8_t>(rdramb, address) != (val & 0xFF); }));
        }
        else if (opcode == "D3")
        {
            // Word inequality comparison
            compiled_cheat.instructions.emplace_back(
                std::make_tuple(true, [=] { return core_rdram_load<uint16_t>(rdramb, address) != val; }));
        }
        else if (opcode == "50")
        {
            // Enter serial mode, which writes multiple bytes for example
            serial = true;
            serial_count = (address & 0xFF00) >> 8;
            serial_offset = address & 0xFF;
            serial_diff = val;
        }
        else
        {
            g_core->log_error(std::format("[GS] Illegal instruction {}\n", opcode));
            return false;
        }
    }

    compiled_cheat.code = code;
    cheat = compiled_cheat;

    return true;
}

bool cht_read_from_file(const std::filesystem::path &path, std::vector<core_cheat> &cheats)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        g_core->log_error(std::format("cht_read_from_file failed to open file {}", path.string()));
        return false;
    }

    file.seekg(0, std::ios::end);
    const size_t len = file.tellg();
    file.seekg(0, std::ios::beg);

    bool reading_cheat_code = false;
    cheats.clear();

    for (auto line : IOUtils::iter_lines(file))
    {
        if (line.empty()) continue;

        if (reading_cheat_code && !cheats.empty())
        {
            std::format_to(std::back_inserter(cheats.back().code), "{}\n", line);
        }

        if (line.starts_with("--"))
        {
            reading_cheat_code = true;
            core_cheat cheat{};
            cheat.name = line.substr(2);
            cheats.emplace_back(cheat);
        }
    }

    for (auto &cheat : cheats)
    {
        // We need to patch up the names since the cheats are reconstructed when compiling
        const auto name = cheat.name;
        core_cht_compile(cheat.code, cheat);
        cheat.name = name;
    }

    return true;
}

std::string cht_serialize()
{
    std::scoped_lock lock(cheats_mutex);

    if (host_cheats.empty())
    {
        return "";
    }

    std::string str;
    for (const auto &cheat : host_cheats)
    {
        constexpr auto CHEAT_FORMAT = "--{}\n"
                                      "{}\n";
        std::format_to(std::back_inserter(str), CHEAT_FORMAT, cheat.name, cheat.code);
    }

    return str;
}

void core_cht_get_override_stack(std::stack<std::vector<core_cheat>> &stack)
{
    std::scoped_lock lock(cheats_mutex);

    stack = cheat_stack;
}

void cht_get_list(std::vector<core_cheat> &list)
{
    std::scoped_lock lock(cheats_mutex);

    list = cheat_stack.empty() ? host_cheats : cheat_stack.top();
}

void core_cht_set_list(const std::vector<core_cheat> &list)
{
    std::scoped_lock lock(cheats_mutex);

    if (!cheat_stack.empty())
    {
        g_core->log_warn(std::format("core_cht_set_list ignored due to cheat stack not being empty"));
        return;
    }

    host_cheats = list;
}

void cht_layer_push(const std::vector<core_cheat> &cheats)
{
    std::scoped_lock lock(cheats_mutex);

    g_core->log_info(std::format("cht_layer_push pushing {} cheats", cheats.size()));

    cheat_stack.push(cheats);
}

void cht_layer_pop()
{
    std::scoped_lock lock(cheats_mutex);

    if (!cheat_stack.empty())
    {
        cheat_stack.pop();
    }
}

void cht_execute()
{
    std::scoped_lock lock(cheats_mutex);

    const auto &cheats = cheat_stack.empty() ? host_cheats : cheat_stack.top();

    for (const auto &cheat : cheats)
    {
        if (!cheat.active)
        {
            return;
        }

        bool execute = true;
        for (auto instruction : cheat.instructions)
        {
            if (execute)
            {
                execute = std::get<1>(instruction)();
            }
            else if (!std::get<0>(instruction))
            {
                execute = true;
            }
        }
    }
}
