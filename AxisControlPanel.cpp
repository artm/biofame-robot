#include "AxisControlPanel.h"
#include "ui_AxisControlPanel.h"

#include "Motor.h"

AxisControlPanel::AxisControlPanel(QWidget *parent)
    : QGroupBox(parent)
    , ui(new Ui::AxisControlPanel)
    , m_motor(0)
{
    ui->setupUi(this);
    m_inputs = new QButtonGroup(this);
    m_outputs = new QButtonGroup(this);

    m_inputs->addButton(ui->in0, 0);
    m_inputs->addButton(ui->in1, 1);
    m_inputs->addButton(ui->in2, 2);
    m_inputs->addButton(ui->in3, 3);
    m_inputs->addButton(ui->in4, 4);
    m_inputs->addButton(ui->in5, 5);
    m_inputs->addButton(ui->in6, 6);
    m_inputs->addButton(ui->in7, 7);
    m_inputs->setExclusive(false);

    m_outputs->addButton(ui->out0, 0);
    m_outputs->addButton(ui->out1, 1);
    m_outputs->addButton(ui->out2, 2);
    m_outputs->addButton(ui->out3, 3);
    m_outputs->addButton(ui->out4, 4);
    m_outputs->addButton(ui->out5, 5);
    m_outputs->addButton(ui->out6, 6);
    m_outputs->addButton(ui->out7, 7);
    m_outputs->setExclusive(false);
    for(int i=4; i<8; ++i)
        connect(m_outputs->button(i), SIGNAL(toggled(bool)),SLOT(syncOutputs()));

    connect(ui->cw, SIGNAL(clicked()), SLOT(goCw()));
    connect(ui->ccw, SIGNAL(clicked()), SLOT(goCcw()));
    connect(ui->stop, SIGNAL(clicked()), SLOT(stop()));
    connect(ui->setAxisPara, SIGNAL(clicked()), SLOT(setAxisPara()));

    connect(ui->initSpeed, SIGNAL(editingFinished()), SLOT(enableSetAxisPara()));
    connect(ui->driveSpeed, SIGNAL(editingFinished()), SLOT(enableSetAxisPara()));
    connect(ui->maxDriveSpeed, SIGNAL(editingFinished()), SLOT(enableSetAxisPara()));
    connect(ui->acceleration, SIGNAL(editingFinished()), SLOT(enableSetAxisPara()));
    connect(ui->accelerationRate, SIGNAL(editingFinished()), SLOT(enableSetAxisPara()));
}

AxisControlPanel::~AxisControlPanel()
{
    delete ui;
}

void AxisControlPanel::setMotor(Motor *motor)
{
    m_motor = motor;
    m_motor->setUi(this);
}

void AxisControlPanel::output(int idx, bool state)
{
    Q_ASSERT( idx >= 4 && idx <= 7 );
    m_outputs->button(idx)->setChecked(state);
}

void AxisControlPanel::syncOutputs()
{
    if (!m_motor) return;

    quint8 mask = 0;
    for(int i=4; i<8; ++i) {
        if (m_outputs->button(i)->isChecked())
            mask |= (1 << (i-4));
    }
    m_motor->output(mask);
}

void AxisControlPanel::goCw()
{
    if (!m_motor) return;
    m_motor->goCw();
}

void AxisControlPanel::goCcw()
{
    if (!m_motor) return;
    m_motor->goCcw();
}

void AxisControlPanel::stop()
{
    if (!m_motor) return;
    m_motor->stop();
}

void AxisControlPanel::onBoardOpened()
{
    output(4,true);

    // ask the registers...
    ui->initSpeed->setValue( m_motor->getReg(0x301) );
    ui->driveSpeed->setValue( m_motor->getReg(0x302));
    ui->maxDriveSpeed->setValue( m_motor->getReg(0x303));
    ui->acceleration->setValue( m_motor->getReg(0x304));
    ui->accelerationRate->setValue( m_motor->getReg(0x306));
}

void AxisControlPanel::onBoardClosing()
{
    output(4,false);
}

void AxisControlPanel::onBoardClosed()
{
}

void AxisControlPanel::enableSetAxisPara()
{
    ui->setAxisPara->setEnabled(true);
}

void AxisControlPanel::disableSetAxisPara()
{
    ui->setAxisPara->setEnabled(false);
}

void AxisControlPanel::setAxisPara()
{
    if (!m_motor) return;
    m_motor->setAxisPara(
                (bool)ui->curveType->currentIndex(),
                ui->initSpeed->value(),
                ui->driveSpeed->value(),
                ui->maxDriveSpeed->value(),
                ui->acceleration->value(),
                ui->accelerationRate->value());
    disableSetAxisPara();
}

void AxisControlPanel::displayPosition(int position)
{
    ui->position->setValue(position);
}

void AxisControlPanel::displaySpeed(int speed)
{
    ui->speed->setValue(speed);
}

void AxisControlPanel::poll()
{
    if (!m_motor) return;

    quint8 di = m_motor->inputs();
    for(int i=0; i<8; ++i)
        m_inputs->button(i)->setChecked( (di & (1<<i)) != 0 );
}
