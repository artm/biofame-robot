#include <QtGui/QApplication>
#include <QMessageBox>
#include <QtDebug>
#include "RoboShell.h"

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName("V2_ Lab");
    QCoreApplication::setOrganizationDomain("v2.nl");
    QCoreApplication::setApplicationName("BioFame");

    QApplication a(argc, argv);
    RoboShell shell;
    shell.show();

    return a.exec();
}
