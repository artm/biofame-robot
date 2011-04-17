#ifndef ROBOSHELL_H
#define ROBOSHELL_H

#include <QMainWindow>
#include <QStateMachine>

class FaceTracker;

namespace Ui {
    class RoboShell;
}

class videoInput;

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

    explicit RoboShell(QWidget *parent = 0);
    ~RoboShell();

    void buildStateMachine();
    static RoboShell * instance() { return s_shell; }

signals:
    void boardOpened();
    void boardClosing();
    void boardClosed();

    void faceDetected(QPointF force);

public slots:
    void closeMotors();
    void openMotors(int id = 0);
    void toggleOpenMotors(bool on);
    void stopAllAxes();
    void motorsTask();

    void videoTask();
    void openCamera();
    void openCamSettings();

    void log(QtMsgType type, const char * message);

protected:
    virtual bool eventFilter(QObject *, QEvent *);

    Ui::RoboShell *ui;
    int m_boardId;
    QTimer m_pollTimer, m_videoTimer;

    QStateMachine * m_automaton;

    videoInput * m_cams;
    QImage m_frame;

    FaceTracker * m_faceTracker;
    int m_openCam;

    static void msgHandler(QtMsgType type, const char * message);
    static RoboShell * s_shell;
    static QtMsgHandler s_oldMsgHandler;
};

#endif // ROBOSHELL_H
