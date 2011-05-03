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

# fmod
INCLUDEPATH += $$quote(C:/Program Files/FMOD SoundSystem/FMOD Programmers API Win32/api/inc)
LIBS += -L$$quote(C:/Program Files/FMOD SoundSystem/FMOD Programmers API Win32/api/lib)
LIBS += -lfmodex

INCLUDEPATH += $$quote($$PWD/../sdk/boost_1_44_0/include)

OTHER_FILES += DEVLOG.txt
