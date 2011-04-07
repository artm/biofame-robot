QT       += core gui

TARGET = BioFame
TEMPLATE = app

SOURCES += main.cpp\
        RoboShell.cpp

HEADERS  += RoboShell.h \
    stable.h

FORMS    += RoboShell.ui

PRECOMPILED_HEADER = stable.h
