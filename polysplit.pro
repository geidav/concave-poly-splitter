#-------------------------------------------------
#
# Project created by QtCreator 2015-01-07T09:03:57
#
#-------------------------------------------------

QT       += core gui

CONFIG += c++11

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = polysplit
TEMPLATE = app


SOURCES += main.cpp\
        mainwnd.cpp \
    polysplitter.cpp

HEADERS  += mainwnd.hpp \
    polysplitter.hpp

FORMS    += mainwnd.ui
