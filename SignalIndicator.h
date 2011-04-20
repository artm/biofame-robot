#ifndef SIGNALINDICATOR_H
#define SIGNALINDICATOR_H

#include <QWidget>

// displays a floating point value constrained to [-1;1[ range (symmetric) or [0;1] (asymmetric)
// as a small bar
class SignalIndicator : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(bool symmetric READ isSymmetric WRITE setSymmetric)
public:
    explicit SignalIndicator(QWidget *parent = 0);

    bool isSymmetric() const { return m_symmetric; }
signals:

public slots:
    void setSymmetric(bool sym) { m_symmetric = sym; update(); }
    void setValue(float v) { m_value = v; update(); }
    void setValue(double v) { setValue((float)v); }

protected:
    void paintEvent(QPaintEvent *);

    bool m_symmetric;
    float m_value;
};

#endif // SIGNALINDICATOR_H
