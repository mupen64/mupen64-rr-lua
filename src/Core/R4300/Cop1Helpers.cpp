/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <Core.hpp>
#include <R4300/R4300.hpp>
#include <R4300/Exception.hpp>
#include <R4300/Cop1Helpers.hpp>

float largest_denormal_float = 1.1754942106924411e-38f;  // (1U << 23) - 1
double largest_denormal_double = 2.225073858507201e-308; // (1ULL << 52) - 1

constexpr auto FLOAT_EXCEPTION_MSG = "A floating point exception has occured in the core. This error is likely "
                                     "unrecoverable.\n\n{} (PC = {:#06x})\n\nHow would you like to proceed?";

static void fail_float(std::string_view msg)
{
    const auto message = std::format(FLOAT_EXCEPTION_MSG, msg, interpcore ? interp_addr : PC->addr);
    const auto choice = g_core->show_multiple_choice_dialog(
        CORE_DLG_FLOAT_EXCEPTION, {"Close ROM", "Continue"}, message.c_str(), "Core", fsvc_error);

    core_Cause = 15 << 2;
    exception_general();

    if (choice == 0)
    {
        g_core->submit_task([] { g_ctx.vr_close_rom(true); });
    }
}

void fail_float_input()
{
    fail_float("Operation on denormal/nan");
}

void fail_float_input_arg(double x)
{
    fail_float(std::format("Operation on denormal/nan: {}", x));
}

void fail_float_output()
{
    fail_float("Float operation resulted in nan");
}

void fail_float_convert()
{
    fail_float("Out-of-range float conversion");
}
