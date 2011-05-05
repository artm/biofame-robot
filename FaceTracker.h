#ifndef FACETRACKER_H
#define FACETRACKER_H

#include <QObject>

class Trackable;

class FaceTracker : public QObject {
    Q_OBJECT
public:
    explicit FaceTracker(QObject * parent);
    virtual ~FaceTracker();
    void findFaces(const QImage& frame, QList<QRect>& faces);

    bool usesVerilook() const { return m_extractor != 0; }

    Trackable * startTracking(const QImage& frame, const QRect& face);
    bool track(const QImage &frame, Trackable * trackable);
    QRect trackableRect(const Trackable * trackable);

public slots:
    /* verilook specific */
    void setMinIOD(int value);
    void setMaxIOD(int value);
    void setConfidenceThreshold(double value);
    void setQualityThreshold(int value);

    void setSMin( int smin ) { m_smin = smin; }
    void setVMin( int vmin ) { m_smin = vmin; }
    void setVMax( int vmax ) { m_smin = vmax; }

private:
    /* verilook specific */
    HNLExtractor m_extractor;

    static QString errorString(NResult result);
    static bool isOk(NResult result,
                     QString errorSuffix = QString(),
                     QString successMessage = QString());

    /* opencv specific */
    cv::CascadeClassifier * m_cvDetector;

    /* trackable */
    Trackable * m_trackable;
    int m_smin, m_vmin, m_vmax;
};


#endif // FACETRACKER_H
