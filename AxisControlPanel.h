#ifndef AXISCONTROLPANEL_H
#define AXISCONTROLPANEL_H

#include <QGroupBox>

namespace Ui {
    class AxisControlPanel;
}

class AxisControlPanel : public QGroupBox
{
    Q_OBJECT

public:
    explicit AxisControlPanel(QWidget *parent = 0);
    ~AxisControlPanel();

private:
    Ui::AxisControlPanel *ui;
};

#endif // AXISCONTROLPANEL_H
