QT += core gui

TARGET = BioFame
TEMPLATE = app

SOURCES += main.cpp\
        RoboShell.cpp \
    AxisControlPanel.cpp \
    Motor.cpp \
    FaceTracker.cpp \
    SignalIndicator.cpp \
    SoundSystem.cpp

HEADERS  += RoboShell.h \
    stable.h \
    AxisControlPanel.h \
    Motor.h \
    FaceTracker.h \
    SignalIndicator.h \
    SoundSystem.h

FORMS    += RoboShell.ui \
    AxisControlPanel.ui

PRECOMPILED_HEADER = stable.h

# motors
INCLUDEPATH += $$quote(C:/Program Files/Advantech/Motion/PCI-1240/Examples/Include)
# unlike MinGW 4.5 (that I use when building with cmake)
# the version of MinGW that comes with QtSDK (MinGW 4.4) can't
# link against ADS1240.dll directly. Hence linking with this (interface
# library or something like that).
LIBS += $$quote(C:/Program Files/Advantech/Motion/PCI-1240/Examples/VC/LIB/ADS1240.lib)

# video input
INCLUDEPATH += $$quote($$PWD/../videoInput)
LIBS += -L$$quote($$PWD/../build/videoInput-mingw) -lvideoInput
LIBS += -luuid -lstrmiids -lole32 -loleaut32

# verilook
INCLUDEPATH += $$quote(C:/Program Files/Neurotechnology/VeriLook 3.2 Standard SDK/include/Windows)
LIBS += -L$$quote(C:/Program Files/Neurotechnology/VeriLook 3.2 Standard SDK/lib/Win32_x86)
LIBS += -lNLExtractor.dll -lNMatcher.dll -lNTemplate.dll -lNCore.dll -lNImages.dll -lNLicensing.dll

# OpenCV (what about OpenCV's own cmake files?)
INCLUDEPATH += $$quote($$PWD/../sdk/OpenCV-2.2.0/include/opencv)
INCLUDEPATH += $$quote($$PWD/../sdk/OpenCV-2.2.0/modules/calib3d/include)
INCLUDEPATH += $$quote($$PWD/../sdk/OpenCV-2.2.0/modules/contrib/include)
INCLUDEPATH += $$quote($$PWD/../sdk/OpenCV-2.2.0/modules/core/include)
INCLUDEPATH += $$quote($$PWD/../sdk/OpenCV-2.2.0/modules/features2d/include)
INCLUDEPATH += $$quote($$PWD/../sdk/OpenCV-2.2.0/modules/flann/include)
INCLUDEPATH += $$quote($$PWD/../sdk/OpenCV-2.2.0/modules/gpu/include)
INCLUDEPATH += $$quote($$PWD/../sdk/OpenCV-2.2.0/modules/highgui/include)
INCLUDEPATH += $$quote($$PWD/../sdk/OpenCV-2.2.0/modules/imgproc/include)
INCLUDEPATH += $$quote($$PWD/../sdk/OpenCV-2.2.0/modules/legacy/include)
INCLUDEPATH += $$quote($$PWD/../sdk/OpenCV-2.2.0/modules/ml/include)
INCLUDEPATH += $$quote($$PWD/../sdk/OpenCV-2.2.0/modules/objdetect/include)
INCLUDEPATH += $$quote($$PWD/../sdk/OpenCV-2.2.0/modules/video/include)
LIBS += -L$$PWD/../sdk/OpenCV-build-win32/3rdparty/lib
LIBS += -L$$PWD/../sdk/OpenCV-build-win32/lib
# on windows opencv libraries have 220 (version number) attached to filename :(
#LIBS += -lopencv_calib3d220
#LIBS += -lopencv_contrib220
#LIBS += -lopencv_features2d220
#LIBS += -lopencv_flann220
#LIBS += -lopencv_ffmpeg220
#LIBS += -lopencv_gpu220
#LIBS += -lopencv_haartraining_engine
#LIBS += -lopencv_highgui220
#LIBS += -lopencv_legacy220
#LIBS += -lopencv_ml220
#LIBS += -lopencv_video220
LIBS += -lopencv_objdetect220
LIBS += -lopencv_imgproc220
LIBS += -lopencv_core220
LIBS += -llibjasper -llibjpeg -llibpng -lopencv_lapack -lzlib

# fmod
INCLUDEPATH += $$quote(C:/Program Files/FMOD SoundSystem/FMOD Programmers API Win32/api/inc)
LIBS += -L$$quote(C:/Program Files/FMOD SoundSystem/FMOD Programmers API Win32/api/lib)
LIBS += -lfmodex

# Boost (but there is boost support in cmake!)
INCLUDEPATH += $$quote($$PWD/../sdk/boost_1_44_0/include)

OTHER_FILES += DEVLOG.txt
