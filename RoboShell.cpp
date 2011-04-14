#include "RoboShell.h"
#include "ui_RoboShell.h"

#include "Motor.h"

#include <QtCore>

RoboShell::RoboShell(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::RoboShell)
    , m_boardId(-1)
{
    ui->setupUi(this);
    connect(ui->openControllerButton, SIGNAL(toggled(bool)),SLOT(toggleOpen(bool)));

    ui->cameraPanel->setMotor( new Motor(m_boardId, CAMERA) );
    ui->armPanel->setMotor( new Motor(m_boardId, ARM) );
    ui->bodyPanel->setMotor( new Motor(m_boardId, BODY) );
    ui->wheelsPanel->setMotor( new Motor(m_boardId, WHEELS) );

    ui->openControllerButton->setChecked(true);
}

RoboShell::~RoboShell()
{
    delete ui;
    if (m_boardId >= 0)
        close();
}

void RoboShell::close()
{
    if (m_boardId < 0)
        return;

    ui->cameraPanel->output(4,false);
    ui->armPanel->output(4,false);
    ui->bodyPanel->output(4,false);
    ui->wheelsPanel->output(4,false);

    if (P1240MotDevClose(m_boardId) == ERROR_SUCCESS) {
        qDebug() << "Successfully closed board";
    }
    m_boardId = -1;
}

void RoboShell::open(int id)
{
    Q_ASSERT( id >= 0 && id <= 15 );
    close();
    if (P1240MotDevOpen(id) != ERROR_SUCCESS) {
        qWarning() << "Motor controller wasn't open";
        return;
    }
    m_boardId = id;
    ui->cameraPanel->output(4,true);
    ui->armPanel->output(4,true);
    ui->bodyPanel->output(4,true);
    ui->wheelsPanel->output(4,true);
}

void RoboShell::toggleOpen(bool on)
{
    if (on)
        open();
    else
        close();

    ui->openControllerButton->setChecked( m_boardId >= 0 );
}
