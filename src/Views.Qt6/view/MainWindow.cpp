/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "MainWindow.hpp"
#include <iostream>

#include <QAction>
#include <QCheckBox>
#include <QMainWindow>
#include <QPushButton>
#include <QString>

#include "moc_MainWindow.cpp"

MainWindow::MainWindow(QMainWindow *parent) : QMainWindow(parent)
{
    ui.setupUi(this);
    connect(ui.actOpenRom, &QAction::triggered, this, &MainWindow::onOpenRom);
    connect(ui.actCloseRom, &QAction::triggered, this, &MainWindow::onCloseRom);
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
std::pair<size_t, bool> MainWindow::showInfoDialog(const QString &title, const QString &text, QMessageBox::Icon icon)
{
    auto messageBox = QMessageBox(icon, title, text, QMessageBox::Ok, this);
    messageBox.exec();
}

void MainWindow::onOpenRom(bool state)
{
    std::cout << "open rom\n";
    ui.pager->setCurrentIndex(1);
}

void MainWindow::onCloseRom(bool state)
{
    std::cout << "close rom\n";
    ui.pager->setCurrentIndex(0);
}