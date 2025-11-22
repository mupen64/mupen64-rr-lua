#ifndef UI_MAIN_WINDOW_HPP_INCLUDED
#define UI_MAIN_WINDOW_HPP_INCLUDED

#include <QMainWindow>
#include <QMessageBox>
#include <QString>
#include "ui_MainWindow.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT
  public:
    MainWindow(QMainWindow *parent = 0);

    Q_INVOKABLE std::pair<size_t, bool> showChoiceDialog(const std::vector<QString> &choices, const QString &title,
                                                         const QString &message, QMessageBox::Icon icon);

    Q_INVOKABLE std::pair<size_t, bool> showInfoDialog(const QString &title, const QString &message,
                                                       QMessageBox::Icon icon);

  private slots:
    void onOpenRom(bool state);

    void onCloseRom(bool state);

  private:
    Ui::MainWindow ui;
};

#endif