#-------------------------------------------------

# Project created by QtCreator 2017-10-17T23:43:01
#
#-------------------------------------------------

QT       += core gui
CONFIG   += c++11

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = qtcreatortest
TEMPLATE = app

DESTDIR = $$PWD/build
OBJECTS_DIR = $$PWD/build/.obj
SRCPATH = $$PWD/src
LIBBWCNC_SRCPATH = $$PWD/../libbwcnc

INCLUDEPATH += /usr/include/eigen3 \
               $$LIBBWCNC_SRCPATH

SOURCES +=  $$SRCPATH/main.cpp \
            $$SRCPATH/mainwindow.cpp \
            $$SRCPATH/qpixmaprenderer.cpp \
            $$SRCPATH/mainwindowhexgrid.cpp \


HEADERS  += $$SRCPATH/mainwindow.h \
            $$SRCPATH/qpixmaprenderer.h \
            $$SRCPATH/stdtforms.h


SOURCES +=  $$LIBBWCNC_SRCPATH/boundingbox.cpp \
            $$LIBBWCNC_SRCPATH/color.cpp \
            $$LIBBWCNC_SRCPATH/command.cpp \
            $$LIBBWCNC_SRCPATH/functions.cpp \
            $$LIBBWCNC_SRCPATH/hexgrid.cpp \
            $$LIBBWCNC_SRCPATH/numstring.cpp \
            $$LIBBWCNC_SRCPATH/part.cpp \
            $$LIBBWCNC_SRCPATH/renderer.cpp

HEADERS  += $$LIBBWCNC_SRCPATH/boundingbox.h \
            $$LIBBWCNC_SRCPATH/color.h \
            $$LIBBWCNC_SRCPATH/command.h \
            $$LIBBWCNC_SRCPATH/functions.h \
            $$LIBBWCNC_SRCPATH/hexgrid.h \
            $$LIBBWCNC_SRCPATH/numstring.h \
            $$LIBBWCNC_SRCPATH/part.h \
            $$LIBBWCNC_SRCPATH/renderer.h \
            $$LIBBWCNC_SRCPATH/bwcnc.h

FORMS    += src/mainwindow.ui

