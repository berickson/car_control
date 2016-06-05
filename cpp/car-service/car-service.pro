TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt
CONFIG += c++14
CONFIG += thread
SOURCES += main.cpp \
    ackerman.cpp \
    usb.cpp \
    dynamics.cpp \
    car.cpp \
    config.cpp \
    work_queue.cpp

HEADERS += \
    geometry.h \
    ackerman.h \
    usb.h \
    work_queue.h \
    dynamics.h \
    split.h \
    car.h \
    config.h \
    trim.h
