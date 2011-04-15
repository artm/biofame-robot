#ifndef AXISCONTROLPANEL_H
#define AXISCONTROLPANEL_H

#include <QGroupBox>
#include <QButtonGroup>
#include <QState>

namespace Ui {
    class AxisControlPanel;
}

class Motor;

class AxisControlPanel : public QGroupBox
{
    Q_OBJECT

public:
    explicit AxisControlPanel(QWidget *parent = 0);
    ~AxisControlPanel();

    void setMotor(Motor * motor);
    Motor * motor() { return m_motor; }

    void output(int idx, bool state);

    // state machine primitives
    void insertCircleCalibState(QState * parent);
    void insertSeekState(QState * parent);

signals:
    void in6_0(); // input 6 transition from 1 to 0
    void driveFinished();
    void positionChanged(int pos);
    void haveForce();

public slots:
    void onBoardOpened();
    void onBoardClosing();
    void onBoardClosed();

    void syncOutputs();
    void goCw();
    void goCcw();
    void stop();

    void enableSetAxisPara();
    void disableSetAxisPara();
    void setAxisPara();

    void poll();
    // normally the axis will poll and display it own registers
    // display slots are for registers that might be polled by the other modules
    void displayPosition(int position);
    void displaySpeed(int speed);

    void resetPosition();
    void posToCircleLength();

    void track(double force);
    void trackX(QPointF force);
    void trackY(QPointF force);

    // try to move so the other axis moves towards desired angle
    void trackAxis(int position);

    void moveToForce();

    void parseEvents(quint8 mask);

    void setTrackCoeff(double coeff);

    void checkForce();

private:
    Ui::AxisControlPanel *ui;
    QButtonGroup * m_inputs, * m_outputs;
    Motor * m_motor;
    int m_circleLength;
    quint8 m_cachedInputs;
    double m_trackingForce;

    double m_trackingCoeff;
};

#endif // AXISCONTROLPANEL_H
