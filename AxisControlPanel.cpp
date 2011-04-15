#include "AxisControlPanel.h"
#include "ui_AxisControlPanel.h"

#include "RoboShell.h"
#include "Motor.h"

#include <QFinalState>

AxisControlPanel::AxisControlPanel(QWidget *parent)
    : QGroupBox(parent)
    , ui(new Ui::AxisControlPanel)
    , m_motor(0)
    , m_circleLength(0)
    , m_cachedInputs(0)
    , m_trackCoeff(100.0)
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

    connect(ui->trackingCoeff, SIGNAL(valueChanged(double)), SLOT(setTrackCoeff(double)));
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
    qDebug() << "go CW!";
    m_motor->goCw();
}

void AxisControlPanel::goCcw()
{
    if (!m_motor) return;
    qDebug() << "go CCW!";
    m_motor->goCcw();
}

void AxisControlPanel::stop()
{
    if (!m_motor) return;
    qDebug() << "stop!";
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

    m_motor->enableEvents(0xff); // all events
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
    for(int i=0; i<8; ++i) {
        quint8 bit = di & (1<<i);
        quint8 old = m_cachedInputs & (1<<i);
        if (bit == old)
            continue;
        m_inputs->button(i)->setChecked( bit );
        if ((i==6) && old)
            emit in6_0();
    }
    m_cachedInputs = di;

    // check interrupt events...
}

void AxisControlPanel::insertCircleCalibState(QState *parent)
{
    QState * calib = new QState(parent);

    QState  * s1 = new QState(calib),
            * s2 = new QState(calib);
    QFinalState
            * s3 = new QFinalState(calib);

    calib->setInitialState( s1 );

    s1->addTransition(this, SIGNAL(in6_0()), s2);
    s2->addTransition(this, SIGNAL(in6_0()), s3);

    connect(s1,SIGNAL(entered()),this,SLOT(goCw()));
    connect(s2,SIGNAL(entered()),this,SLOT(resetPosition()));
    connect(s3,SIGNAL(entered()),this,SLOT(posToCircleLength()));
}

void AxisControlPanel::insertSeekState(QState *parent)
{
    QState * seek = new QState(parent);
    QState * s1 = new QState(seek), * s2 = new QState(seek);
    seek->setInitialState(s1);

    s1->addTransition(RoboShell::instance(), SIGNAL(faceDetected(QPointF)), s2 );
    s2->addTransition(this, SIGNAL(driveFinished()), s1 );

    connect(s2, SIGNAL(entered()), this, SLOT(moveToForce()));
}

void AxisControlPanel::resetPosition()
{
    if (!m_motor) return;
    qDebug() << "resetting position to 0";
    m_motor->setReg(Lcnt, 0);
}

void AxisControlPanel::posToCircleLength()
{
    if (!m_motor) return;
    m_circleLength = m_motor->getReg(Lcnt);
    qDebug() << "new circle length:" << m_circleLength;
}

void AxisControlPanel::track(QPointF force)
{
    m_trackingForce = force;
}

void AxisControlPanel::moveToForce()
{
    if (!m_motor) return;
    int dx = (int)m_trackCoeff * m_trackingForce.x();
    m_motor->rmove(dx);
}

void AxisControlPanel::parseEvents(quint8 mask)
{
    if (mask & 0x80) emit driveFinished();
}

