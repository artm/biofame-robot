#ifndef MOTOR_H
#define MOTOR_H

#include <QObject>
#include <QAbstractTransition>
#include "AxisControlPanel.h"

class Motor : public QObject
{
    Q_OBJECT
public:
    enum EventId {
        STOPPED,
    };

    struct Event : public QEvent {
        Event(int axis, EventId id)
            : QEvent(QEvent::Type(QEvent::User+1))
            , m_axis(axis)
            , m_id(id)
        {}

        int axis() const { return m_axis; }
        EventId id() const { return m_id; }

    protected:

        int m_axis;
        EventId m_id;
    };


    explicit Motor(int& boardId, int axisNum, QObject *parent = 0);
    void setUi(AxisControlPanel * ui) { setParent(ui); m_ui = ui; }

    int getReg(int reg);
    void setReg(int reg, int value);

    quint8 inputs();
signals:

public slots:
    void output(quint8 mask);
    void cmove(bool cw);
    void rmove(int dx);
    void goCw() { cmove(true); }
    void goCcw() { cmove(false); }
    void stop();

    void setAxisPara(bool sCurve, int initSpeed, int driveSpeed, int maxDriveSpeed,
                     int acceleration, int accelerationRate);

    void enableEvents(quint8 mask);

protected:
    int& m_boardId;
    int m_axisBit;

    AxisControlPanel * m_ui;
};

class MotorEventTransition : public QAbstractTransition {
    Q_OBJECT
public:
    MotorEventTransition(int axis, Motor::EventId id);
protected:
    virtual bool eventTest(QEvent *event);
    virtual void onTransition(QEvent *) {}

    int m_axis;
    Motor::EventId m_id;
};

#endif // MOTOR_H
