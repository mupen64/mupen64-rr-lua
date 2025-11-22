#include "MainWindow.hpp"
#include <iostream>

#include <QAction>
#include <QPushButton>
#include <QMainWindow>

#include "moc_MainWindow.cpp"

MainWindow::MainWindow(QMainWindow *parent) : QMainWindow(parent)
{
    ui.setupUi(this);
    connect(ui.actOpenRom, &QAction::triggered, this, &MainWindow::onOpenRom);
    connect(ui.actCloseRom, &QAction::triggered, this, &MainWindow::onCloseRom);
}

std::pair<size_t, bool> MainWindow::showChoiceDialog(const std::vector<QString>& choices, const QString &title,
                                                     const QString &message, QMessageBox::Icon icon)
{
  choices | std::views::transform([](const QString& choice) {
    return QPushButton("yeet");
  });
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