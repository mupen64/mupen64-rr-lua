/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "MainWindow.hpp"

#include <QAction>
#include <QCheckBox>
#include <QFileDialog>
#include <QMainWindow>
#include <QPushButton>
#include <QString>

// #include "moc_MainWindow.cpp"

#include <boost/dll/runtime_symbol_info.hpp>
#include <chrono>
#include <qopenglcontext.h>
#include <qthread.h>
#include <thread>

#include "../model/Core.hpp"
#include "../model/Logging.hpp"
#include "../model/Plugin.hpp"
#include "render/GLRenderWindow.hpp"

void MainWindow::onOpenRom(bool state)
{
    m_openRomDialog->open();
}
void MainWindow::onOpenRom1(const QString &qsPath)
{
    ui.pager->setCurrentIndex(1);

    auto exeDir = std::filesystem::path(boost::dll::program_location().parent_path().c_str());
    auto pluginDir = exeDir / "plugin";
    auto hardcodedPlugins = Mupen::PluginPaths{
        .video_path = pluginDir / "no-video.so",
        .audio_path = pluginDir / "audio-sdl.so",
        .input_path = pluginDir / "no-input.so",
        .rsp_path = pluginDir / "TASRSP.so",
    };

    m_glRenderTest.reset(new GLRenderWindow(ui.gameWindowPage));

    m_glRenderTest->container()->move(0, 0);
    m_glRenderTest->container()->resize(640, 480);
    m_glRenderTest->show();

    {
        GLRenderWindow *glrw = m_glRenderTest.get();
        QOpenGLContext *ctx = m_glRenderTest->context();
        m_glTestThread.reset(new std::thread([glrw, ctx]() {
            while (!glrw->isExposed())
            {
                std::this_thread::yield();
            }
            ctx->makeCurrent(glrw);
            auto glClearColor = (void (*)(float, float, float, float))ctx->getProcAddress("glClearColor");
            auto glClear = (void (*)(uint32_t))ctx->getProcAddress("glClear");

            for (uint64_t i = 0; i < 2000; i++)
            {
                ctx->makeCurrent(glrw);

                glClearColor(1.0, 0.0, 0.0, 1.0);
                glClear(GL_COLOR_BUFFER_BIT);
                ctx->swapBuffers(glrw);

                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }));
    }

    auto path = QFileInfo(qsPath).filesystemFilePath();
    Mupen::core_start(path, hardcodedPlugins);
}

void MainWindow::onCloseRom(bool state)
{
    Mupen::core_stop();
    m_glTestThread->join();
    m_glTestThread.reset();
    m_glRenderTest.reset();
    ui.pager->setCurrentIndex(0);
}