#ifndef STABLE_H
#define STABLE_H

#if defined __cplusplus
#include <QtCore>
#include <QtGui>
#include <QtDebug>

#include <boost/assign/list_of.hpp>

#include "videoInput.h"

#endif

extern "C" {
#include <windows.h>
#include <windef.h>

#include <Ads1240.h>

#include <NCore.h>
#include <NLExtractor.h>
#include <NLicensing.h>

}

#endif // STABLE_H
