#ifndef MOTOR_H
#define MOTOR_H

#include <QObject>
#include "AxisControlPanel.h"

class Motor : public QObject
{
    Q_OBJECT
public:
    explicit Motor(int& boardId, int axisNum, QObject *parent = 0);
    void setUi(AxisControlPanel * ui) { setParent(ui); m_ui = ui; }

    int getReg(int reg);
    void setReg(int reg, int value);

    quint8 inputs();
signals:

public slots:
    void output(quint8 mask);
    void cmove(bool cw);
    void goCw() { cmove(true); }
    void goCcw() { cmove(false); }
    void stop();

    void setAxisPara(bool sCurve, int initSpeed, int driveSpeed, int maxDriveSpeed,
                     int acceleration, int accelerationRate);

protected:
    int& m_boardId;
    int m_axisBit;

    AxisControlPanel * m_ui;
};

#endif // MOTOR_H
