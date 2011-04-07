#include "RoboShell.h"
#include "ui_RoboShell.h"

RoboShell::RoboShell(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::RoboShell)
{
    ui->setupUi(this);
}

RoboShell::~RoboShell()
{
    delete ui;
}
