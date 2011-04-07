#include "AxisControlPanel.h"
#include "ui_AxisControlPanel.h"

AxisControlPanel::AxisControlPanel(QWidget *parent) :
    QGroupBox(parent),
    ui(new Ui::AxisControlPanel)
{
    ui->setupUi(this);
}

AxisControlPanel::~AxisControlPanel()
{
    delete ui;
}
