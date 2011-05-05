#include "QtOpenCVHelpers.h"

namespace QtCv
{

cv::Mat QImage2CvMat(const QImage &img)
{
    int cvType;

    switch (img.format()) {
    case QImage::Format_Indexed8:
        cvType = CV_8UC1;
        break;
    case QImage::Format_RGB888:
        cvType = CV_8UC3;
        break;
    default:
        qCritical() << "Error: unsupported conversion from QImage to cv::Mat";
        return cv::Mat();
    }

    // using this constructor:
    // Mat(int _rows, int _cols, int _type, void* _data, size_t _step=AUTO_STEP);
    return cv::Mat(img.height(), img.width(), cvType, (void*)img.bits(), img.bytesPerLine());
}

cv::Mat QImage2CvMat(const QImage& img, QRect ROI)
{
    cv::Mat full = QImage2CvMat(img);
    return cv::Mat(full, QRect2CvRect(ROI));
}


cv::Rect QRect2CvRect(const QRect& rect)
{
    return cv::Rect( rect.x(), rect.y(), rect.width(), rect.height() );
}

QRect CvRect2QRect(const cv::Rect& rect)
{
    return QRect(rect.x,rect.y,rect.width,rect.height);
}

} // namespace QtCv
