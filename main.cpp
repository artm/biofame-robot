#include <QtGui/QApplication>
#include "RoboShell.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    RoboShell w;
    w.show();

    return a.exec();
}
