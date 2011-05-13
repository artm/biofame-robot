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

static bool s_greyTableInit = false;
static QVector<QRgb> s_greyTable;

QImage CvMat2QImage(const cv::Mat& cvmat)
{
    int height = cvmat.rows;
    int width = cvmat.cols;

    if (cvmat.depth() == CV_8U && cvmat.channels() == 3) {
        QImage img((const uchar*)cvmat.data, width, height, cvmat.step.p[0], QImage::Format_RGB888);
        return img.rgbSwapped();
    } else if (cvmat.depth() == CV_8U && cvmat.channels() == 1) {
        if (!s_greyTableInit) {
            for (int i = 0; i < 256; i++){
                s_greyTable.push_back(qRgb(i, i, i));
            }
        }
        QImage img((const uchar*)cvmat.data, width, height, cvmat.step.p[0], QImage::Format_Indexed8);
        img.setColorTable(s_greyTable);
        return img;
    } else {
        qWarning() << "Image cannot be converted.";
        return QImage();
    }
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
