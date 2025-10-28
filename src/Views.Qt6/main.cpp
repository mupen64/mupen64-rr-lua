
#include <qapplication.h>
#include "MainWindow.hpp"

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);
  MainWindow mainWindow;
  mainWindow.show();

  return QApplication::exec();
}