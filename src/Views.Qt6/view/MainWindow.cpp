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

#include "moc_MainWindow.cpp"

#include <boost/dll/runtime_symbol_info.hpp>

#include "../model/Core.hpp"
#include "../model/Logging.hpp"
#include "../model/Plugin.hpp"

MainWindow::MainWindow(QMainWindow *parent)
    : QMainWindow(parent), m_openRomDialog(new QFileDialog(this, tr("Open ROM...")))
{
    using namespace Qt::Literals;
    ui.setupUi(this);

    m_openRomDialog->setFileMode(QFileDialog::ExistingFile);
    m_openRomDialog->setNameFilter(u"N64 ROM (*.n64 *.v64 *.z64)"_s);

    connect(ui.actOpenRom, &QAction::triggered, this, &MainWindow::onOpenRom);
    connect(ui.actCloseRom, &QAction::triggered, this, &MainWindow::onCloseRom);
    connect(m_openRomDialog.get(), &QFileDialog::fileSelected, this, &MainWindow::onOpenRom1);
}

std::pair<size_t, bool> MainWindow::showChoiceDialog(const std::vector<QString> &choices, const QString &title,
                                                     const QString &text, QMessageBox::Icon icon)
{
    // list of push buttons for choices (to be checked after)
    auto buttonList = std::vector<QPushButton *>{};
    buttonList.reserve(choices.size());

    // setup the dialog
    auto messageBox = QMessageBox(icon, title, text, QMessageBox::NoButton, this);

    // setup buttons and checkbox
    for (auto &choice : choices)
    {
        auto choiceBtn = new QPushButton(choice);
        buttonList.push_back(choiceBtn);
        messageBox.addButton(choiceBtn, QMessageBox::NoRole);
    }

    auto choiceCheckbox = new QCheckBox(tr("Don't show again"));
    messageBox.setCheckBox(choiceCheckbox);

    // show the dialog
    messageBox.exec();

    size_t index = std::ranges::find(buttonList, messageBox.clickedButton()) - buttonList.begin();
    bool dontShowAgain = choiceCheckbox->isChecked();
    return {index, dontShowAgain};
}
void MainWindow::showInfoDialog(const QString &title, const QString &text, QMessageBox::Icon icon)
{
    auto messageBox = QMessageBox(icon, title, text, QMessageBox::Ok, this);
    messageBox.exec();
}

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
        .audio_path = pluginDir / "audio-pan64.so",
        .input_path = pluginDir / "no-input.so",
        .rsp_path = pluginDir / "TASRSP.so",
    };

    auto path = QFileInfo(qsPath).filesystemFilePath();
    Mupen::core_start(path, hardcodedPlugins);
}

void MainWindow::onCloseRom(bool state)
{
    Mupen::core_stop();
    ui.pager->setCurrentIndex(0);
}