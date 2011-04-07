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

private:
    Ui::RoboShell *ui;
};

#endif // ROBOSHELL_H
