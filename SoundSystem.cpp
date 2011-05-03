#include "SoundSystem.h"

#include <QTimer>

#include <fmod.h>
#include <fmod_errors.h>

struct SoundSystemPrivate {
    FMOD_SYSTEM * system;
    FMOD_SOUND * click;

    SoundSystemPrivate()
        : system(0)
        , click(0)
    {}
};

inline bool fmod_check_helper(FMOD_RESULT result, const QString& file, int line)
{
    if (result == FMOD_OK)
        return true;
    qCritical() << QString("Fmod error: %1 @ %2:%3")
                   .arg(FMOD_ErrorString(result))
                   .arg(file).arg(line)
                   .toAscii().constData();
    return false;
}

#define FMOD_CHECK(expr) (fmod_check_helper(expr,__FILE__,__LINE__))

SoundSystem::SoundSystem(QObject *parent)
    : QThread(parent)
    , m_minGeigerPeriod(25)
    , m_maxGeigerPeriod(1000)
    , m_newInterval(-1)
    , m_private(new SoundSystemPrivate)
{
    if (!FMOD_CHECK( FMOD_System_Create(&m_private->system))) {
        qDebug() << "No sound system created";
        return;
    }
    if (!FMOD_CHECK(FMOD_System_Init(m_private->system, 64, FMOD_INIT_NORMAL , 0))) {
        qDebug() << "Failed to init sound system";
        return;
    }
    qDebug() << "FMOD initialized";

    if (!FMOD_CHECK( FMOD_System_CreateSound( m_private->system, "Tick.wav", FMOD_INIT_NORMAL , 0,
                                             &m_private->click)))
        qDebug() << "No click loaded";
    else {
        connect(&m_geiger, SIGNAL(timeout()), SLOT(click()));
        setGeiger(0.0);
    }
}

SoundSystem::~SoundSystem()
{
    if (m_private->system)
        FMOD_CHECK(FMOD_System_Release(m_private->system));
}

void SoundSystem::run()
{
    exec();
}

void SoundSystem::click()
{
    if (!m_private || !m_private->system || !m_private->click)
        exit();
    FMOD_CHANNEL * channel;
    FMOD_CHECK( FMOD_System_PlaySound(m_private->system, FMOD_CHANNEL_FREE,
                                      m_private->click, false, &channel) );
    FMOD_System_Update( m_private->system );
    if (m_newInterval >= 0) {
        m_geiger.start(m_newInterval);
        m_newInterval = -1;
    }
}

void SoundSystem::setGeiger(int value)
{
    if (!value) {
        if (m_geiger.isActive())
            m_geiger.stop();
    } else {
        m_newInterval = m_minGeigerPeriod
                + (1.0 - std::min(1.0, std::max(0.0, 0.01 * value)))
                * (m_maxGeigerPeriod-m_minGeigerPeriod);
        if (!m_geiger.isActive()) {
            m_geiger.start(m_newInterval);
            m_newInterval = -1;
        }
    }

}
