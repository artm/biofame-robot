#include "FaceTracker.h"

#include <QtDebug>
#include <QtCore>

bool FaceTracker::s_gotLicense = false;
int FaceTracker::s_refcount = 0;
const NWChar * FaceTracker::s_defaultPort = (NWChar*)"5000";
const NWChar * FaceTracker::s_defaultServer = (NWChar*)"/local";
const NWChar * FaceTracker::s_licenseList = (NWChar*)
"SingleComputerLicense:VLExtractor,SingleComputerLicense:VLMatcher";

FaceTracker * FaceTracker::make()
{
    if (!obtainLicense())
        return 0;
    s_refcount++;
    return new FaceTracker();
}

FaceTracker::FaceTracker()
    : m_detectEyes( true ), m_recognize(false)

{
    if (! isOk(NleCreate(&m_extractor),
               "No verilook extractor created",
               "Verilook extractor created") ) {
        m_extractor = 0;
    }
}

FaceTracker::~FaceTracker() {
    if (m_extractor) {
        NleFree(m_extractor);
        m_extractor = 0;
    }

    if (--s_refcount == 0) {
        releaseLicense();
    }
}

bool FaceTracker::obtainLicense()
{
    if (!s_gotLicense) {
        NBool available;

        if ( !isOk( NLicenseObtain(s_defaultServer, s_defaultPort,
                                   s_licenseList, &available),
                   "NLicenseObtain failed") )
            return false;


        if (!available) {
            qWarning() << QString("License for %1 not available.")
                          .arg((char*)s_licenseList);
        } else {
            qDebug() << "License successfully obtained.";
            s_gotLicense = true;
        }
    }

    return s_gotLicense;
}

void FaceTracker::releaseLicense()
{
    if (!s_gotLicense) return;

    if ( isOk( NLicenseRelease((NWChar*)s_licenseList),
              "NLicenseRelease failed",
              "License successfully released" ) ) {
        s_gotLicense = false;
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


void FaceTracker::process(const QImage& frame, QList<QRect>& faces)
{
#if 0
    MutexTryLocker locker(m_extractorMutex);

    if (!locker)
        return;

    HNImage img;
    if ( !isOk( NImageCreateWrapper(
                   npfGrayscale,
                   frame.cols, frame.rows, frame.step.p[0],
                   0.0, 0.0, frame.data, NFalse, &img),
               "Coudn't wrap matrix for verilook"))
        return;

    /* detect faces */
    int faceCount = 0;
    NleFace * vlFaces;
    NleDetectFaces( m_extractor, img, &faceCount, &vlFaces);


    int maxArea = 0;
    NleDetectionDetails maxDetails;

    // convert to mirror faces
    for(int i = 0; i<faceCount; ++i) {
        Face face( QRectF(vlFaces[i].Rectangle.X,
                          vlFaces[i].Rectangle.Y,
                          vlFaces[i].Rectangle.Width,
                          vlFaces[i].Rectangle.Height));
        if (m_detectEyes || m_recognize) {
            NleDetectionDetails vlDetails;
            if ( isOk(NleDetectFacialFeatures(m_extractor, img,
                                              &vlFaces[i],
                                              &vlDetails))) {
                if (vlDetails.EyesAvailable == NTrue) {
                    face.setEyes(QPointF(vlDetails.Eyes.First.X,
                                         vlDetails.Eyes.First.Y),
                                 QPointF(vlDetails.Eyes.Second.X,
                                         vlDetails.Eyes.Second.Y));
                    int area =
                            vlFaces[i].Rectangle.Width *
                            vlFaces[i].Rectangle.Height;
                    if (area > maxArea) {
                        maxArea = area;
                        maxDetails = vlDetails;
                    }
                }
            }
        }

        faces.push_back( face );
    }

    if (maxArea > 0
            && m_matchingThread
            && m_recognize
            && !m_matchingThread->isRunning()) {
        // pass the largest face to the recognizer
        m_matchingThread->setupRecognition( img, maxDetails );
        m_matchingThread->start();
    }

    NFree(vlFaces);
#endif
}


