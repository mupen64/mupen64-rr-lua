/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <Core.h>
#include <memory/memory.h>
#include <cheats.h>
#include <r4300/r4300.h>

static std::recursive_mutex cheats_mutex;
static std::vector<core_cheat> host_cheats;
static std::stack<std::vector<core_cheat>> cheat_stack;

bool core_cht_compile(const std::wstring& code, core_cheat& cheat)
{
    core_cheat compiled_cheat{};

    auto lines = split_string(code, L"\n");

    bool serial = false;
    size_t serial_count = 0;
    size_t serial_offset = 0;
    size_t serial_diff = 0;

    for (const auto& line : lines)
    {
        if (line[0] == '$' || line[0] == '-' || line.size() < 13)
        {
            g_core->log_info(L"[GS] Line skipped");
            continue;
        }

        auto opcode = line.substr(0, 2);

        uint32_t address = std::stoul(line.substr(2, 6), nullptr, 16);
        uint32_t val;
        if (line.substr(8, 1) == L" ")
        {
            val = std::stoul(line.substr(9, 4), nullptr, 16);
        }
        else
        {
            val = std::stoul(line.substr(10, 4), nullptr, 16);
        }

        if (serial)
        {
            g_core->log_info(std::format(L"[GS] Compiling {} serial byte writes...", serial_count));

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

        if (opcode == L"80" || opcode == L"A0")
        {
            // Write byte
            compiled_cheat.instructions.emplace_back(std::make_tuple(false, [=] {
                core_rdram_store<uint8_t>(rdramb, address, val & 0xFF);
                return true;
            }));
        }
        else if (opcode == L"81" || opcode == L"A1")
        {
            // Write word
            compiled_cheat.instructions.emplace_back(std::make_tuple(false, [=] {
                core_rdram_store<uint16_t>(rdramb, address, val);
                return true;
            }));
        }
        else if (opcode == L"88")
        {
            // Write byte if GS button pressed
            compiled_cheat.instructions.emplace_back(std::make_tuple(false, [=] {
                if (core_vr_get_gs_button())
                {
                    core_rdram_store<uint8_t>(rdramb, address, val & 0xFF);
                }
                return true;
            }));
        }
        else if (opcode == L"89")
        {
            // Write word if GS button pressed
            compiled_cheat.instructions.emplace_back(std::make_tuple(false, [=] {
                if (core_vr_get_gs_button())
                {
                    core_rdram_store<uint16_t>(rdramb, address, val);
                }
                return true;
            }));
        }
        else if (opcode == L"D0")
        {
            // Byte equality comparison
            compiled_cheat.instructions.emplace_back(std::make_tuple(true, [=] {
                return core_rdram_load<uint8_t>(rdramb, address) == (val & 0xFF);
            }));
        }
        else if (opcode == L"D1")
        {
            // Word equality comparison
            compiled_cheat.instructions.emplace_back(std::make_tuple(true, [=] {
                return core_rdram_load<uint16_t>(rdramb, address) == val;
            }));
        }
        else if (opcode == L"D2")
        {
            // Byte inequality comparison
            compiled_cheat.instructions.emplace_back(std::make_tuple(true, [=] {
                return core_rdram_load<uint8_t>(rdramb, address) != (val & 0xFF);
            }));
        }
        else if (opcode == L"D3")
        {
            // Word inequality comparison
            compiled_cheat.instructions.emplace_back(std::make_tuple(true, [=] {
                return core_rdram_load<uint16_t>(rdramb, address) != val;
            }));
        }
        else if (opcode == L"50")
        {
            // Enter serial mode, which writes multiple bytes for example
            serial = true;
            serial_count = (address & 0xFF00) >> 8;
            serial_offset = address & 0xFF;
            serial_diff = val;
        }
        else
        {
            g_core->log_error(std::format(L"[GS] Illegal instruction {}\n", opcode.c_str()));
            return false;
        }
    }

    compiled_cheat.code = code;
    cheat = compiled_cheat;

    return true;
}

bool cht_read_from_file(const std::filesystem::path& path, std::vector<core_cheat>& cheats)
{
    FILE* f = nullptr;

    if (_wfopen_s(&f, path.wstring().c_str(), L"r"))
    {
        g_core->log_error(std::format(L"cht_read_from_file failed to open file {}", path.wstring()));
        return false;
    }

    fseek(f, 0, SEEK_END);
    const auto len = ftell(f);
    fseek(f, 0, SEEK_SET);

    std::string str;
    str.resize(len + sizeof(char));
    fread(str.data(), sizeof(char), len, f);

    fclose(f);

    std::wstring wstr = string_to_wstring(str);

    const auto lines = split_string(wstr, L"\n");

    bool reading_cheat_code = false;
    cheats.clear();

    for (const auto& line : lines)
    {
        if (line.empty())
        {
            continue;
        }

        if (reading_cheat_code && !cheats.empty())
        {
            auto& cheat = cheats.back();
            cheat.code += line + L"\n";
        }

        if (line.starts_with(L"--"))
        {
            reading_cheat_code = true;
            core_cheat cheat{};
            cheat.name = line.substr(2);
            cheats.emplace_back(cheat);
        }
    }

    for (auto& cheat : cheats)
    {
        // We need to patch up the names since the cheats are reconstructed when compiling
        const auto name = cheat.name;
        core_cht_compile(cheat.code, cheat);
        cheat.name = name;
    }

    return true;
}

void core_cht_get_override_stack(std::stack<std::vector<core_cheat>>& stack)
{
    std::scoped_lock lock(cheats_mutex);

    stack = cheat_stack;
}

void core_cht_get_list(std::vector<core_cheat>& list)
{
    std::scoped_lock lock(cheats_mutex);
    
    list = cheat_stack.empty() ? host_cheats : cheat_stack.top();
}

void core_cht_set_list(const std::vector<core_cheat>& list)
{
    std::scoped_lock lock(cheats_mutex);

    if (!cheat_stack.empty())
    {
        g_core->log_warn(std::format(L"core_cht_set_list ignored due to cheat stack not being empty"));
        return;
    }

    host_cheats = list;
}

void cht_layer_push(const std::vector<core_cheat>& cheats)
{
    std::scoped_lock lock(cheats_mutex);

    g_core->log_info(std::format(L"cht_layer_push pushing {} cheats", cheats.size()));
    
    cheat_stack.push(cheats);
}

void cht_layer_pop()
{
    std::scoped_lock lock(cheats_mutex);

    cheat_stack.pop();
}

void cht_execute()
{
    std::scoped_lock lock(cheats_mutex);

    const auto& cheats = cheat_stack.empty() ? host_cheats : cheat_stack.top();

    for (const auto& cheat : cheats)
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
