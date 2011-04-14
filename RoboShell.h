#ifndef ROBOSHELL_H
#define ROBOSHELL_H

#include <QMainWindow>

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

    explicit RoboShell(QWidget *parent = 0);
    ~RoboShell();

signals:
    void boardOpened();
    void boardClosing();
    void boardClosed();

public slots:
    void close();
    void open(int id = 0);
    void toggleOpen(bool on);

    void poll();

private:
    Ui::RoboShell *ui;
    int m_boardId;
    QTimer * m_pollTimer;
};

#endif // ROBOSHELL_H
