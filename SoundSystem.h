#ifndef SOUNDSYSTEM_H
#define SOUNDSYSTEM_H

#include <QThread>

struct SoundSystemPrivate;

class SoundSystem : public QThread
{
    Q_OBJECT
public:
    explicit SoundSystem(QObject *parent = 0);
    ~SoundSystem();
    virtual void run();

signals:

public slots:
    void setGeigerMinPeriod(int ms) { m_minGeigerPeriod = ms; }
    void setGeigerMaxPeriod(int ms) { m_maxGeigerPeriod = ms; }
    void setGeiger(int value); // 0(silent), 1(max period), ..., 100 (min period)
    void click();

private:
    int m_minGeigerPeriod, m_maxGeigerPeriod;
    int m_newInterval;
    QTimer m_geiger;
    SoundSystemPrivate * m_private;
};

#endif // SOUNDSYSTEM_H
