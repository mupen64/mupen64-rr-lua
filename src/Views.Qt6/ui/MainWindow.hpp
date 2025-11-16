#ifndef UI_MAIN_WINDOW_HPP_INCLUDED
#define UI_MAIN_WINDOW_HPP_INCLUDED

#include <QtGui>
#include <memory>
#include "ui_MainWindow.h"

class MainWindow : public QMainWindow {
  Q_OBJECT
public:
  MainWindow(QMainWindow* parent = 0);

private slots:
  void onOpenRom(bool state);

  void onCloseRom(bool state);

private:
  Ui::MainWindow ui;
  std::unique_ptr<QWindow> m_emuTarget;
};

#endif