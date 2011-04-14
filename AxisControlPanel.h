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

signals:
    void in6_0(); // input 6 transition from 1 to 0

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

private:
    Ui::AxisControlPanel *ui;
    QButtonGroup * m_inputs, * m_outputs;
    Motor * m_motor;
    int m_circleLength;
    quint8 m_cachedInputs;
};

#endif // AXISCONTROLPANEL_H
