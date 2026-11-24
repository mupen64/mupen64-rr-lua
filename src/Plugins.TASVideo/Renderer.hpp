/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once
#include "DrawList.hpp"
#include "VI.hpp"
#include <atomic>
#include <deque>
#include <future>
#include <memory>
#include <mutex>
#include <span>
#include <thread>

#define RENDER_CLOSE 0
#define RENDER_START 1
#define RENDER_DESTROYTEXTURES 5
#define RENDER_INITTEXTURES 6
#define RENDER_READPIXELS 7
#define RENDER_RESTART 8
#define RENDER_BLACKOUT 9

struct RenderMessage
{
    u32 command;
    void *pixels;
    std::promise<void> completed;
};

struct Renderer
{
    std::mutex poolMutex;
    std::vector<std::unique_ptr<DrawList>> pool;
    std::atomic<std::shared_ptr<const DrawList>> latest;
    std::atomic<u64> sequence{};
    std::mutex mutex;
    std::condition_variable msg_available;
    std::unique_ptr<std::jthread> thread;
    std::deque<RenderMessage> messages;
    u32 listNumber{}, cullBoth{}, cullBack{};
    std::span<const u8> background;
    ~Renderer();
};
extern Renderer renderer;
extern SPDrawState renderSP;
extern gDPInfo renderDP;
extern VIInfo renderVI;
extern std::array<u64, 512> renderTMEM;
void Render_ThreadProc();
void Render_SendMessage(u32 command, void *pixels = nullptr);
void Render_Submit(DrawList &list);
