#ifndef STABLE_H
#define STABLE_H

#if defined __cplusplus
#include <QtCore>
#include <QtGui>
#include <QtDebug>

#include <boost/assign/list_of.hpp>

#include <videoInput.h>

#include <opencv2/objdetect/objdetect.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#endif

extern "C" {
#include <NCore.h>
#include <NLExtractor.h>
#include <NLicensing.h>

#include <windows.h>
#include <windef.h>

#include <Ads1240.h>

#include <fmod.h>
#include <fmod_errors.h>
}

#endif // STABLE_H
