#-------------------------------------------------
#
# Project created by QtCreator 2013-10-29T14:25:51
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = IUNB
TEMPLATE = app


SOURCES += \
    src/main.cpp \
    src/iunb.cpp \
    src/log_pass.cpp

HEADERS  += \
    src/iunb.h \
    src/log_pass.h

FORMS    += \
    src/iunb.ui \
    src/log_pass.ui

INCLUDEPATH += D:\Code\boost_1_54_0
LIBS += -LD:\Code\boost_1_54_0\stage\lib
