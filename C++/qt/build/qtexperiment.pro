#-------------------------------------------------

# Project created by QtCreator 2017-10-17T23:43:01
#
#-------------------------------------------------

QMAKE_CC = clang
QMAKE_CXX = clang++
QMAKE_LINK = clang++

QT       += core gui
CONFIG   += c++11 debug
#CONFIG   += c++11

# profiler
#QMAKE_CXXFLAGS_DEBUG += -pg
QMAKE_CXXFLAGS_DEBUG += -O3
#QMAKE_LFLAGS_DEBUG += -pg

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = qtcreatortest
TEMPLATE = app

DESTDIR = $$PWD
OBJECTS_DIR = $$PWD/.obj
MOC_DIR = $$PWD/.moc
SRCPATH = $$PWD/../src
LIBBWCNC_SRCPATH = $$PWD/../../libbwcnc

INCLUDEPATH += /usr/include/eigen3 \
               $$PWD/../../

#               $$LIBBWCNC_SRCPATH

SOURCES +=  $$SRCPATH/main.cpp \
            $$SRCPATH/mainwindow.cpp \
            $$SRCPATH/mainwindowhexgrid.cpp \
            $$SRCPATH/xhatchwaves.cpp \
            $$SRCPATH/cylinder.cpp

HEADERS  += $$SRCPATH/mainwindow.h


SOURCES +=  $$LIBBWCNC_SRCPATH/boundingbox.cpp \
            $$LIBBWCNC_SRCPATH/color.cpp \
            $$LIBBWCNC_SRCPATH/command.cpp \
            $$LIBBWCNC_SRCPATH/functions.cpp \
            $$LIBBWCNC_SRCPATH/hexgrid.cpp \
            $$LIBBWCNC_SRCPATH/mceschliz.cpp \
            $$LIBBWCNC_SRCPATH/numstring.cpp \
            $$LIBBWCNC_SRCPATH/part.cpp \
            $$LIBBWCNC_SRCPATH/renderer.cpp \
            $$LIBBWCNC_SRCPATH/concurrent.cpp \
            $$LIBBWCNC_SRCPATH/qpixmaprenderer.cpp

HEADERS  += $$LIBBWCNC_SRCPATH/boundingbox.h \
            $$LIBBWCNC_SRCPATH/color.h \
            $$LIBBWCNC_SRCPATH/command.h \
            $$LIBBWCNC_SRCPATH/functions.h \
            $$LIBBWCNC_SRCPATH/hexgrid.h \
            $$LIBBWCNC_SRCPATH/mceschliz.h \
            $$LIBBWCNC_SRCPATH/numstring.h \
            $$LIBBWCNC_SRCPATH/part.h \
            $$LIBBWCNC_SRCPATH/renderer.h \
            $$LIBBWCNC_SRCPATH/qpixmaprenderer.h \
            $$LIBBWCNC_SRCPATH/stdtforms.h \
            $$LIBBWCNC_SRCPATH/concurrent.h \
            $$LIBBWCNC_SRCPATH/bwcnc.h

FORMS    += ../src/mainwindow.ui

