#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
#ifdef _WIN32
  qputenv("QT_QPA_PLATFORM", "windows:darkmode=0");
#endif
  QApplication a(argc, argv);
#ifdef _WIN32
  a.setStyle("windowsvista");
#endif
  MainWindow w;
  w.show();
  return a.exec();
}
