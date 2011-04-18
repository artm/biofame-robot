#include "SignalIndicator.h"

#include <QPainter>
#include <math.h>

#define CLAMP(v, min, max) ((v < min) ? min : (v > max) ? max : v);

SignalIndicator::SignalIndicator(QWidget *parent)
    : QWidget(parent)
    , m_symmetric(false)
    , m_value(0.0)
{
    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);
}

void SignalIndicator::paintEvent(QPaintEvent *)
{
    float v =  CLAMP(m_value, (m_symmetric ? -1 : 0), 1);

    QPainter painter(this);
    int x,w;
    if (m_symmetric) {
        x = width() / 2;
        w = ceil(fabs(v) * width() * 0.5f);
        if (v < 0.0f)
            x -= w;
    } else {
        x = 0;
        w = ceil(v * width());
    }
    if (!w) w = 1;

    painter.fillRect(x,0,w,height(),Qt::black);
}
