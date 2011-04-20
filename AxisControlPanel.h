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
    Q_PROPERTY( bool circleReset READ circleReset WRITE setCircleReset )
public:
    explicit AxisControlPanel(QWidget *parent = 0);
    ~AxisControlPanel();

    void setMotor(Motor * motor);
    Motor * motor() { return m_motor; }

    void output(int idx, bool state);

    // state machine primitives
    void setupCircleCalibState(QState * parent);
    void setupSeekState(QState * parent);
    void setupInitCircleState(QState * parent);

    bool circleReset() const { return m_circleReset; }
    void setCircleReset(bool reset) { m_circleReset = reset; }

    // return estimated angle (-180,180) or 360 for wheels
    double estimatedAngle() const;

signals:
    void in6_fall(); // input 6 transition from 1 to 0
    void in6_raise(); // input 6 transition from 0 to 1
    void driveFinished();
    void positionChanged(int pos);
    void haveForce();

    void angleChanged(double newAngle); // degrees

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
    void posToCircleOffset();
    void posToCircleLength();

    void track(double force);
    void trackX(QPointF force);
    void trackY(QPointF force);

    // try to move so the other axis moves towards desired angle
    void trackAxis(int position);
    void trackAxisDirection(double angle);

    void moveToForce();

    void parseEvents(quint8 mask);

    void setTrackCoeff(double coeff);

    void checkForce();

    void saveSettings(QSettings& s, const QString& group);
    void loadSettings(QSettings& s, const QString& group);

private:
    Ui::AxisControlPanel *ui;
    QButtonGroup * m_inputs, * m_outputs;
    Motor * m_motor;
    int m_circleLength, m_circleOffset; // in pulses
    bool m_circleReset;
    quint8 m_cachedInputs;
    double m_trackingForce;

    double m_trackingCoeff;
};

#endif // AXISCONTROLPANEL_H
