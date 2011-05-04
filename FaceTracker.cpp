#include "FaceTracker.h"

#include <QtDebug>
#include <QImage>

#include <NCore.h>
#include <NLExtractor.h>
#include <NLicensing.h>

#include <opencv2/objdetect/objdetect.hpp>
#include <opencv2/imgproc/imgproc.hpp>

const char * s_defaultPort = "5000";
const char * s_defaultServer = "/local";
const char * s_licenseList = "SingleComputerLicense:VLExtractor";

FaceTracker::FaceTracker(QObject * parent)
    : QObject(parent)
    , m_extractor(0)
    , m_cvDetector(0)
{
    NBool available;

    if ( ! isOk( NLicenseObtainA( s_defaultServer, s_defaultPort, s_licenseList, &available),
               "NLicenseObtain failed")
            || ! available
            || ! isOk(NleCreate(&m_extractor),
                   "No verilook extractor created",
                   "Verilook extractor created") ) {
            m_extractor = 0;
            // set up the OpenCV detector
            m_cvDetector = new cv::CascadeClassifier;
            m_cvDetector->load("lbpcascade_frontalface.xml");
        }
}

FaceTracker::~FaceTracker() {
    if (m_extractor) {
        // clean up verilook stuff
        NleFree(m_extractor);
        m_extractor = 0;
        isOk( NLicenseRelease((NWChar*)s_licenseList),
                  "NLicenseRelease failed",
                  "License successfully released" );
    } else {
        // clean up opencv detector
    }
}

QString FaceTracker::errorString(NResult result)
{
    NInt length;
    NChar* message;
    length = NErrorGetDefaultMessage(result, NULL);
    message = (NChar*) malloc(sizeof(NChar) * (length + 1));
    NErrorGetDefaultMessage(result, message);
    QString qmessage((char*)message);
    free(message);
    return qmessage;
}

bool FaceTracker::isOk(NResult result,
                 QString errorSuffix,
                 QString successMessage) {
    if (NFailed(result)) {
        qCritical()
                << QString("VLERR(%1): %2.%3")
                   .arg(result)
                   .arg(errorString(result))
                   .arg( !errorSuffix.isEmpty()
                        ? (" " + errorSuffix + ".") : "");
        return false;
    } else {
        if (!successMessage.isEmpty())
            qDebug() << successMessage;
        return true;
    }
}

bool larger(const QRect& a, const QRect& b)
{
    return a.width()*a.height() > b.width()*b.height();
}

void FaceTracker::findFaces(const QImage& frame, QList<QRect>& faces)
{
    if (m_extractor) {
        HNImage img;
        if ( !isOk( NImageCreateWrapper(
                       npfGrayscale,
                       frame.width(), frame.height(), frame.bytesPerLine(),
                       0.0, 0.0, (void*)frame.bits(), NFalse, &img),
                   "Coudn't wrap matrix for verilook"))
            return;

        /* detect faces */
        int faceCount = 0;
        NleFace * vlFaces;
        NleDetectFaces( m_extractor, img, &faceCount, &vlFaces);

        // convert to rectangles
        for(int i = 0; i<faceCount; ++i) {
            faces.push_back( QRect(vlFaces[i].Rectangle.X,
                                   vlFaces[i].Rectangle.Y,
                                   vlFaces[i].Rectangle.Width,
                                   vlFaces[i].Rectangle.Height));
        }
        NFree(vlFaces);
    } else if (m_cvDetector) {
        // wrap the frame
        cv::Mat cvFrame( frame.height(), frame.width(), CV_8UC1, (void*)frame.bits(), frame.bytesPerLine());
        // detect...
        std::vector<cv::Rect> rects;

        int maxSize = frame.height() * 2 / 3;

        m_cvDetector->detectMultiScale( cvFrame, rects, 1.1, 3,
                                       cv::CascadeClassifier::FIND_BIGGEST_OBJECT
                                       | cv::CascadeClassifier::DO_ROUGH_SEARCH
                                       | cv::CascadeClassifier::DO_CANNY_PRUNING,
                                       cv::Size(10, 10), cv::Size(maxSize, maxSize));
        // convert the results to qt rects
        foreach(cv::Rect r, rects)
            faces.push_back( QRect(r.x, r.y, r.width, r.height) );
    }
    qSort(faces.begin(), faces.end(), larger);
}

void FaceTracker::setMinIOD(int value)
{
    if (!m_extractor) return;
    NInt v = (NInt)value;
    NleSetParameter( m_extractor, NLEP_MIN_IOD, (const void *)&v );
}

void FaceTracker::setMaxIOD(int value)
{
    if (!m_extractor) return;
    NInt v = (NInt)value;
    NleSetParameter( m_extractor, NLEP_MAX_IOD, (const void *)&v );
}

void FaceTracker::setConfidenceThreshold(double value)
{
    if (!m_extractor) return;
    NDouble v = (NDouble)value;
    NleSetParameter( m_extractor, NLEP_FACE_CONFIDENCE_THRESHOLD, (const void *)&v );
}

void FaceTracker::setQualityThreshold(int value)
{
    if (!m_extractor) return;
    NByte v = (NByte)value;
    NleSetParameter( m_extractor, NLEP_FACE_CONFIDENCE_THRESHOLD, (const void *)&v );
}


