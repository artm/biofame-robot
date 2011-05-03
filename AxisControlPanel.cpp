#include "AxisControlPanel.h"
#include "ui_AxisControlPanel.h"

#include "RoboShell.h"
#include "Motor.h"

#include <QFinalState>
#include <QSettings>

#include <math.h>

double shortestArcAngle(double angle) {
    angle = fmod(angle, 360.0 ); // this is betwen -360 and 360
    if (angle > 180.0)
        angle -= 360.0;
    else if (angle < -180.0)
        angle += 360;
    return angle;
}

AxisControlPanel::AxisControlPanel(QWidget *parent)
    : QGroupBox(parent)
    , ui(new Ui::AxisControlPanel)
    , m_motor(0)
    , m_circleLength(0)
    , m_circleOffset(0)
    , m_circleReset(false)
    , m_cachedInputs(0)
    , m_trackingForce(0.0)
    , m_tracking(false)
{
    ui->setupUi(this);

    ui->toolbox->setCurrentIndex(0);

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

    connect(this, SIGNAL(driveFinished()), SLOT(reTrack()));

    setDesireControlsVisible(false);
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

    m_motor->setReg(Lcnt, ui->position->value());

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
    if (ui->position->value() != position) {
        ui->position->setValue(position);
        emit positionChanged(position);
        if (m_circleLength)
            emit angleChanged( estimatedAngle() );
    }
}

void AxisControlPanel::displaySpeed(int speed)
{
    if (speed == ui->speed->value())
        return;
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

        if (i==6) { // circle reference trigger
            if (old) {
                if (m_circleReset && m_motor->lastSetDirection() == Motor::Cw)
                    m_motor->setReg(Lcnt, 0);
                emit in6_fall();
            } else {
                if (m_circleReset && m_motor->lastSetDirection() == Motor::Ccw)
                    m_motor->setReg(Lcnt, m_circleLength);
                emit in6_raise();
            }
        }
    }
    m_cachedInputs = di;

    reTrack();
}

void AxisControlPanel::setupCircleCalibState(QState * calib)
{
    QState  * s1 = new QState(calib),
            * s2 = new QState(calib);
    QFinalState
            * s3 = new QFinalState(calib);

    calib->setInitialState( s1 );

    s1->addTransition(this, SIGNAL(in6_fall()), s2);
    s2->addTransition(this, SIGNAL(in6_fall()), s3);

    connect(s1,SIGNAL(entered()), SLOT(goCw()));
    connect(s2,SIGNAL(entered()), SLOT(posToCircleOffset()));
    connect(s3,SIGNAL(entered()), SLOT(posToCircleLength()));

    calib->assignProperty(this, "circleReset", false);
}

void AxisControlPanel::setupContinuousTracking(QState *parent)
{
    parent->assignProperty(this, "tracking", true);
}

void AxisControlPanel::setupSeekState(QState * seek)
{
    /*
    QState * s1 = new QState(seek), * s2 = new QState(seek);
    seek->setInitialState(s1);

    s1->addTransition(this, SIGNAL(haveForce()), s2);
    s2->addTransition(this, SIGNAL(driveFinished()), s1 );

    connect(s1, SIGNAL(entered()), this, SLOT(checkForce()));
    connect(s2, SIGNAL(entered()), this, SLOT(moveToForce()));
    */

    seek->assignProperty(this, "tracking", true);
    connect(seek, SIGNAL(entered()), SLOT(reTrack()));
}

void AxisControlPanel::setupInitCircleState(QState * init)
{
    QState * s1 = new QState(init);
    QFinalState * s2 = new QFinalState(init);
     init->setInitialState(s1);
    s1->addTransition(this,SIGNAL(in6_fall()), s2);
    connect(s1, SIGNAL(entered()), SLOT(goCw()));
    connect(s2, SIGNAL(entered()), SLOT(stop()));
    setCircleReset(true);
}


void AxisControlPanel::resetPosition()
{
    if (!m_motor) return;
    m_motor->setReg(Lcnt, 0);
}

void AxisControlPanel::posToCircleOffset()
{
    if (!m_motor) return;
    m_circleOffset = - m_motor->getReg(Lcnt);
    resetPosition();
}

void AxisControlPanel::posToCircleLength()
{
    if (!m_motor) return;
    m_circleLength = m_motor->getReg(Lcnt);
    resetPosition();
}

void AxisControlPanel::track(double force)
{
    m_trackingForce = std::max(-1.0, std::min( 1.0, force ) );
    emit forceFeedback(m_trackingForce);

    if (!m_tracking)
        return;

    qDebug() << title() << "tracking with force" << m_trackingForce;

    if (!m_motor) return;

    int desiredSpeed = force * ui->maxDriveSpeed->value() * (ui->invert->isChecked() ? -1 : 1);

    Motor::Direction desiredDirection = (desiredSpeed < 0) ? Motor::Ccw : Motor::Cw;
    desiredSpeed = abs(desiredSpeed);

    if (desiredSpeed < 5) {
        // ensure we stop
        if ((m_motor->motionState() == Motor::MotionCont)
                || (m_motor->motionState() == Motor::MotionPtp))
            stop();
    } else {
        // ensure we move in the right direction and set speed
        m_motor->setSpeed(desiredSpeed); // driving speed
        if ( m_motor->motionState() == Motor::MotionStopped ) {
            m_motor->cmove(desiredDirection);
        } else if ((m_motor->lastSetDirection() != desiredDirection)
                   && m_motor->motionState() != Motor::MotionBreaking) {
            m_motor->stop();
        }
    }
}


void AxisControlPanel::trackX(QPointF force)
{
    track(force.x());
}

void AxisControlPanel::trackY(QPointF force)
{
    track(force.y());
}

void AxisControlPanel::trackAxis(double angle)
{
    track( shortestArcAngle(ui->desire->value() - angle) / 180.0 );
}

void AxisControlPanel::trackAxisDirection(double angle)
{
    double f = angle / 90;
    if (f < -1.0) f = -2.0 - f;
    else if (f > 1.0) f = 2.0 - f;
    track(f);
}

void AxisControlPanel::parseEvents(quint8 mask)
{
    if (mask & 0x80) {
        qDebug() << title() << "drive finished event";
        emit driveFinished();
        if (m_motor)
            m_motor->notifyStopped();
    }
}

void AxisControlPanel::saveSettings(QSettings& s, const QString &group)
{
    s.beginGroup(group);
    s.setValue("desire", ui->desire->value());
    s.setValue("invert", ui->invert->isChecked());

    if (m_circleLength) {
        s.setValue("circleOffset", m_circleOffset);
        s.setValue("circleLength", m_circleLength);
    }

    s.setValue("lcnt", ui->position->value());
    s.endGroup();
}

void AxisControlPanel::loadSettings(QSettings& s, const QString &group)
{
    s.beginGroup(group);
    ui->desire->setValue(s.value("desire", 0).toInt());
    ui->invert->setChecked(s.value("invert", false).toBool());

    m_circleOffset = s.value("circleOffset", 0).toInt();
    m_circleLength = s.value("circleLength", 0).toInt();
    m_circleReset = m_circleLength != 0;

    ui->position->setValue(s.value("lcnt", 0).toInt());

    s.endGroup();
}

double AxisControlPanel::estimatedAngle() const
{
    if (!m_motor)
        return 360.0;

    if (!m_circleReset) {
        return 360.0;
    } else if (!m_circleLength) {
        return 360.0;
    }

    return shortestArcAngle( 360.0 * (m_motor->getReg(Lcnt) + m_circleOffset) / m_circleLength );
}

void AxisControlPanel::setDesireControlsVisible(bool on)
{
    ui->desire->setVisible(on);
    ui->desireLabel->setVisible(on);
}

