#ifndef FACETRACKER_H
#define FACETRACKER_H

#include <NCore.h>
#include <NLExtractor.h>
#include <NLicensing.h>

#include <QObject>

class FaceTracker : public QObject {
    Q_OBJECT
public:
    static FaceTracker * make();
    virtual ~FaceTracker();
    static bool obtainLicense();
    static void releaseLicense();
    static QString errorString(NResult result);
    static bool isOk(NResult result,
                     QString errorSuffix = QString(),
                     QString successMessage = QString());
    void process(const QImage& frame, QList<QRect>& faces);

    bool detectEyes() const { return m_detectEyes; }
    void setDetectEyes(bool on) { m_detectEyes = on; }
    bool recognize() const { return m_recognize; }
    void setRecognize(bool on) { m_recognize = on; }

public slots:

private:
    FaceTracker();

    HNLExtractor m_extractor;

    bool m_detectEyes, m_recognize;

    static bool s_gotLicense;

    static const char * s_defaultPort;
    static const char * s_defaultServer;
    static const char * s_licenseList;

    static int s_refcount;
};

#endif // FACETRACKER_H
