#ifndef ROBOSHELL_H
#define ROBOSHELL_H

#include <QMainWindow>
#include <QStateMachine>
#include <QTextStream>

#include <math.h>

class FaceTracker;
class videoInput;
class SoundSystem;
class Trackable;

namespace Ui {
    class RoboShell;
}

class RoboShell : public QMainWindow
{
    Q_OBJECT

public:
    enum {
        CAMERA,
        ARM,
        BODY,
        WHEELS
    };

    enum TrackerState {
        FACE_DETECTION,
        TRACKING,
    };

    explicit RoboShell(QWidget *parent = 0);
    ~RoboShell();

    void buildStateMachine();
    static RoboShell * instance() { return s_shell; }

    void addNamedState(const QString& tag, QAbstractState* state);

signals:
    void boardOpened();
    void boardClosing();
    void boardClosed();

    void faceDetected(QPointF force);
    void faceLost();
    void gotcha();

public slots:
    void closeMotors();
    void openMotors(int id = 0);
    void toggleOpenMotors(bool on);
    void stopAllAxes();
    void motorsTask();

    void videoTask();
    void openCamera();
    void openCamSettings();

    void loadSettings();
    void saveSettings();
    void log(QtMsgType type, const char * message);

    void updateTimeStamp();
    void resetTracker() { m_trackingState = FACE_DETECTION; }

    void setDisplayMode(int i) { m_displayMode = i; }

    QString printVerbState(const QString& verb);
    void onStateEnter();
    void onStateExit();

    void turnAround();

protected:
    virtual bool eventFilter(QObject *, QEvent *);

    void notifyOfFace(const QRect& face, const QImage& where);
    void machineTick();

    Ui::RoboShell *ui;
    int m_boardId;
    QTimer m_pollTimer, m_videoTimer;

    QStateMachine * m_machine;
    QMap<QString, QAbstractState*> m_states;
    QTimer m_refindTimer, m_stareTimer, m_roamTimer, m_turnAroundTimer;
    QTime m_timeInTrack;

    videoInput * m_cams;
    int m_openCam;
    QImage m_frame;

    FaceTracker * m_faceTracker;
    // new statefull tracking
    TrackerState m_trackingState;

    SoundSystem * m_sound;

    int m_displayMode;

    // detection/tracking state
    QPointF m_faceCenter; // relative to frame center
    float m_faceSize; // relative to frame width
    QTime m_faceTimestamp; // when last seen

    // Logger (TODO separate)
    QFile m_logFile;
    QTextStream m_log;
    static void msgHandler(QtMsgType type, const char * message);
    static RoboShell * s_shell;
    static QtMsgHandler s_oldMsgHandler;
};

#endif // ROBOSHELL_H
