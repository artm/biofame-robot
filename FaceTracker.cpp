#include "FaceTracker.h"
#include "QtOpenCVHelpers.h"

#include <QtDebug>
#include <QImage>

#include <NCore.h>
#include <NLExtractor.h>
#include <NLicensing.h>

#include <opencv2/objdetect/objdetect.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/video/tracking.hpp>

const char * s_defaultPort = "5000";
const char * s_defaultServer = "/local";
const char * s_licenseList = "SingleComputerLicense:VLExtractor";

// histogram settings: only hue
const int g_channels[] = { 0 }; // hue, saturation
const int g_histSize[] = { 10 };
const float g_hranges[] = { 0, 180 };
const float * g_ranges[] = { g_hranges };
const int g_chCount = sizeof(g_channels) / sizeof(int);

FaceTracker::FaceTracker(QObject * parent)
    : QObject(parent)
    , m_extractor(0)
    , m_cvDetector(0)
    , m_smin(30)
    , m_vmin(10)
    , m_vmax(255)
    , m_retrackThreshold(0.2)
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

void FaceTracker::startTracking(const QImage &frame, const QRect &face)
{
    cv::Mat sub = QtCv::QImage2CvMat(frame, face);
    // convert sub-area to HSV
    cv::cvtColor(sub, sub, CV_RGB2HSV);
    // find the HSV histogram
    cv::calcHist( &sub,
             1, // one array
             g_channels,
             cv::Mat(), // do not use mask
             m_trackHistogram,
             g_chCount,
             g_histSize,
             g_ranges,
             true, // the histogram is uniform
             false // don't accumulate
             );

    m_trackWindow = QtCv::QRect2CvRect(face);
}

bool FaceTracker::track(const QImage &frame)
{
    // calculate back projection of trackable's histogram...
    cv::Mat cvFrame = QtCv::QImage2CvMat(frame);
    cv::cvtColor(cvFrame,cvFrame,CV_RGB2HSV);

    cv::Mat mask(cvFrame.rows, cvFrame.cols, CV_8UC1);
    cv::inRange( cvFrame, cv::Scalar(0,m_smin,m_vmin), cv::Scalar(180,255,m_vmax), mask );

    cv::Mat prob(cvFrame.rows, cvFrame.cols, CV_8UC1);
    // mask out things that are too grey
    cv::calcBackProject( &cvFrame, 1, g_channels, m_trackHistogram, prob, g_ranges);
    cv::bitwise_and(prob,mask,prob);

    m_probabilityImage = QtCv::CvMat2QImage(prob);

    // track colors
    cv::RotatedRect rotRect =
            cv::CamShift(prob, m_trackWindow,
                         cv::TermCriteria(cv::TermCriteria::COUNT|cv::TermCriteria::EPS,25,1));

    m_trackWindow = rotRect.boundingRect() & cv::Rect(0,0,cvFrame.cols,cvFrame.rows);
    m_trackConfidence = cv::mean(cv::Mat(prob, m_trackWindow))[0] / 255.0;

    return m_trackConfidence > m_retrackThreshold;
}

QRect FaceTracker::trackWindow() const
{
    return QtCv::CvRect2QRect(m_trackWindow);
}

double FaceTracker::trackConfidence() const
{
    return m_trackConfidence;
}

QImage FaceTracker::probabilityImage()
{
    return m_probabilityImage;
}
