# -------------------------------------------------
# Project created by QtCreator 2010-02-09T08:55:32
# -------------------------------------------------
QT += testlib
TARGET = NeuralNetworkSimulator
TEMPLATE = app
SOURCES += main.cpp \
    mainwindow.cpp \
    ffnetwork.cpp \
    config.cpp \
    networkmanager.cpp
HEADERS += mainwindow.h \
    ffnetwork.h \
    config.h \
    networkmanager.h
FORMS += mainwindow.ui \
    config.ui
INCLUDEPATH += qwt/src
LIBS += -Lqwt/lib \
    -lqwtd6
