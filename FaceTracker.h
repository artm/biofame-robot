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

    void startTracking(const QImage& frame, const QRect& face);
    bool track(const QImage &frame);
    QRect trackWindow() const;
    double trackConfidence() const;

public slots:
    /* verilook specific */
    void setMinIOD(int value);
    void setMaxIOD(int value);
    void setConfidenceThreshold(double value);
    void setQualityThreshold(int value);

    void setSMin( int smin ) { m_smin = smin; }
    void setVMin( int vmin ) { m_smin = vmin; }
    void setVMax( int vmax ) { m_smin = vmax; }
    void setRetrackThreshold( int percent ) { m_retrackThreshold = 0.01 * percent; }

    int smin() const { return m_smin; }
    int vmin() const { return m_vmin; }
    int vmax() const { return m_vmax; }
    int retrackThreshold() const { return m_retrackThreshold; }

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
    int m_smin, m_vmin, m_vmax;
    cv::Rect m_trackWindow;
    cv::MatND m_trackHistogram;
    double m_trackConfidence;
    double m_retrackThreshold;
};


#endif // FACETRACKER_H
