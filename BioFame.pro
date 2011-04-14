QT += core gui

TARGET = BioFame
TEMPLATE = app

SOURCES += main.cpp\
        RoboShell.cpp \
    AxisControlPanel.cpp \
    Motor.cpp

HEADERS  += RoboShell.h \
    stable.h \
    AxisControlPanel.h \
    Motor.h

FORMS    += RoboShell.ui \
    AxisControlPanel.ui

PRECOMPILED_HEADER = stable.h

INCLUDEPATH += "C:/Program Files/Advantech/Motion/PCI-1240/Examples/Include"
# unlike MinGW 4.5 (that I use when building with cmake)
# the version of MinGW that comes with QtSDK (MinGW 4.4) can't
# link against ADS1240.dll directly. Hence linking with this (interface
# library or something like that).
LIBS += "C:/Program Files/Advantech/Motion/PCI-1240/Examples/VC/LIB/ADS1240.lib"

INCLUDEPATH += "../sdk/boost_1_44_0/include"

OTHER_FILES += \
    DEVLOG.txt
