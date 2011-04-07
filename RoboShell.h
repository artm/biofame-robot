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
    explicit RoboShell(QWidget *parent = 0);
    ~RoboShell();

public slots:
    void close();
    void open(int id = 0);
    void toggleOpen(bool on);

private:
    Ui::RoboShell *ui;
    int m_boardId;
};

#endif // ROBOSHELL_H
