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
    Q_PROPERTY( bool tracking READ isTracking WRITE setTracking )
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
    void setCircleReset(bool reset) {
        qDebug() << title() << ".setCircleReset(" << reset << ")";
        m_circleReset = reset;
    }
    bool isTracking() const { return m_tracking; }

    // return estimated angle (-180,180) or 360 for wheels
    double estimatedAngle() const;
    void gotoAngle(double newAngle);

    void setMachine(QStateMachine * machine) { m_machine = machine; }

signals:
    void inputChanged(int input, int newValue);
    void circleResetHappened();
    void driveFinished();
    void positionChanged(int pos);
    void forceFeedback(double force);
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
    // normally the axis will poll and display its own registers
    // display slots are for registers that might be polled by the shell
    void displayPosition(int position);
    void displaySpeed(int speed);

    void resetPosition();

    void track(double force);
    void reTrack() { track(m_trackingForce); }
    void trackX(QPointF force);
    void trackY(QPointF force);

    // try to move so the other axis moves towards desired angle
    void trackAxis(double angle);
    void trackAxisDirection(double angle);

    void parseEvents(quint8 mask);

    void saveSettings(QSettings& s, const QString& group);
    void loadSettings(QSettings& s, const QString& group);

    void setDesireControlsVisible(bool on);
    void setTracking(bool on) { m_tracking = on; }

    void handleInputChanged(int input, int newValue);

private:
    Ui::AxisControlPanel *ui;
    QButtonGroup * m_inputs, * m_outputs;
    Motor * m_motor;
    int m_circleLength, m_circleOffset; // in pulses
    bool m_circleReset;
    quint8 m_previousInputs;
    double m_trackingForce;

    bool m_tracking;

    QStateMachine * m_machine;
    QState
    * m_calibCircleOffsetState,
    * m_calibCircleLengthState;
};

#endif // AXISCONTROLPANEL_H
