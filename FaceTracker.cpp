#include "FaceTracker.h"

#include <QtDebug>
#include <QImage>

bool FaceTracker::s_gotLicense = false;
int FaceTracker::s_refcount = 0;
const char * FaceTracker::s_defaultPort = "5000";
const char * FaceTracker::s_defaultServer = "/local";
const char * FaceTracker::s_licenseList =
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

        if ( !isOk( NLicenseObtainA(
                       s_defaultServer,
                       s_defaultPort,
                       s_licenseList,
                       &available),
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

bool larger(const QRect& a, const QRect& b)
{
    return a.width()*a.height() > b.width()*b.height();
}

void FaceTracker::process(const QImage& frame, QList<QRect>& faces)
{
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

    qSort(faces.begin(), faces.end(), larger);

    NFree(vlFaces);
}

void FaceTracker::setMinIOD(int value)
{
    NInt v = (NInt)value;
    NleSetParameter( m_extractor, NLEP_MIN_IOD, (const void *)&v );
}

void FaceTracker::setMaxIOD(int value)
{
    NInt v = (NInt)value;
    NleSetParameter( m_extractor, NLEP_MAX_IOD, (const void *)&v );
}

void FaceTracker::setConfidenceThreshold(double value)
{
    NDouble v = (NDouble)value;
    NleSetParameter( m_extractor, NLEP_FACE_CONFIDENCE_THRESHOLD, (const void *)&v );
}

void FaceTracker::setQualityThreshold(int value)
{
    NByte v = (NByte)value;
    NleSetParameter( m_extractor, NLEP_FACE_CONFIDENCE_THRESHOLD, (const void *)&v );
}


