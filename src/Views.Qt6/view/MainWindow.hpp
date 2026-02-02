/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef VIEW_MAIN_WINDOW_HPP_INCLUDED
#define VIEW_MAIN_WINDOW_HPP_INCLUDED

#include <QFileDialog>
#include <QMainWindow>
#include <QMessageBox>
#include <QRect>
#include <QSize>
#include <QString>
#include <QThread>
#include <QWidget>

#include <memory>
#include <qobject.h>
#include <qsurfaceformat.h>
#include <qtmetamacros.h>
#include <thread>

#include "render/GLRenderWindow.hpp"
#include "ui_MainWindow.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT
  public:
    MainWindow(QMainWindow *parent = 0);

    Q_INVOKABLE std::pair<size_t, bool> showChoiceDialog(const std::vector<QString> &choices, const QString &title,
                                                         const QString &message, QMessageBox::Icon icon);

    Q_INVOKABLE void showInfoDialog(const QString &title, const QString &message, QMessageBox::Icon icon);

    // Q_INVOKABLE void setupOpenGL(const QSurfaceFormat& surfaceFormat, uint32_t width, uint32_t height);

    

  private slots:
    void onOpenRom(bool state);
    void onOpenRom1(const QString &qsPath);

    void onCloseRom(bool state);

  private:
    Ui::MainWindow ui;
    std::unique_ptr<QFileDialog> m_openRomDialog;
    std::unique_ptr<GLRenderWindow> m_glRenderTest;
    std::unique_ptr<std::thread> m_glTestThread;
};

#endif