#include "MainWindow.hpp"
#include <qaction.h>
#include <qwindow.h>
#include <iostream>
#include "moc_MainWindow.cpp"

MainWindow::MainWindow(QMainWindow* parent) 
  : QMainWindow(parent), m_emuTarget(nullptr) {
    ui.setupUi(this);
    connect(ui.actOpenRom, &QAction::triggered, this, &MainWindow::onOpenRom);
    connect(ui.actCloseRom, &QAction::triggered, this, &MainWindow::onCloseRom);
}

void MainWindow::onOpenRom(bool state) {
  std::cout << "open rom\n";
  ui.pager->setCurrentIndex(1);
  
}

void MainWindow::onCloseRom(bool state) {
  std::cout << "close rom\n";
  ui.pager->setCurrentIndex(0);
}