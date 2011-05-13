#ifndef QTOPENCVHELPERS_H
#define QTOPENCVHELPERS_H

#include <opencv2/core/core.hpp>
#include <QImage>
#include <QRect>

namespace QtCv
{
    cv::Mat QImage2CvMat(const QImage& img);
    cv::Mat QImage2CvMat(const QImage& img, QRect ROI);
    QImage CvMat2QImage(const cv::Mat& mat);
    cv::Rect QRect2CvRect(const QRect& rect);

    QRect CvRect2QRect(const cv::Rect& rect);

    QRect cutOut(const QRect& orig, float mx, float my);
    QRectF cutOut(const QRectF& orig, float mx, float my);
};

#endif // QTOPENCVHELPERS_H
