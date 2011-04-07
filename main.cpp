#include <QtGui/QApplication>
#include <QMessageBox>
#include <QtDebug>
#include "RoboShell.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    RoboShell shell;
    shell.show();

    return a.exec();
}
