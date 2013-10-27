TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
    src/main.cpp \
    src/iunb.cpp

INCLUDEPATH += D:\Code\boost_1_54_0
LIBS += -LD:\Code\boost_1_54_0\stage\lib

HEADERS += \
    src/iunb.h
