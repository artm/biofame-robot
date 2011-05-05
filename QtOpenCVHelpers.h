#ifndef QTOPENCVHELPERS_H
#define QTOPENCVHELPERS_H

#include <opencv2/core/core.hpp>
#include <QImage>
#include <QRect>

namespace QtCv
{
    cv::Mat QImage2CvMat(const QImage& img);
    cv::Mat QImage2CvMat(const QImage& img, QRect ROI);
    cv::Rect QRect2CvRect(const QRect& rect);

    QRect CvRect2QRect(const cv::Rect& rect);
};

#endif // QTOPENCVHELPERS_H
