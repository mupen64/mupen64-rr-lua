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
#include <qthread.h>

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

    auto path = QFileInfo(qsPath).filesystemFilePath();
    Mupen::core_start(path, hardcodedPlugins);
}

void MainWindow::onCloseRom(bool state)
{
    Mupen::core_stop();
    m_glRenderTest.reset();
    ui.pager->setCurrentIndex(0);

    m_glRenderTest.reset();
}