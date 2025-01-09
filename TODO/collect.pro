DESTDIR = ./build/bin
OBJECTS_DIR = ./build/obj
MOC_DIR = ./build/moc
RCC_DIR = ./build/rcc
UI_DIR = ./build/ui

QT       += core gui concurrent

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

greaterThan(QT_MAJOR_VERSION, 4): CONFIG += c++11
lessThan(QT_MAJOR_VERSION, 5): QMAKE_CXXFLAGS += -std=c++11

LIBS += -L/lib/aarch64-linux-gnu -lcurl
LIBS += -L/lib/aarch64-linux-gnu -lpaho-mqtt3c
LIBS += -L/lib/aarch64-linux-gnu -lqrencode
LIBS += -L/lib/aarch64-linux-gnu -lpng

TARGET = collect
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp\
        qcustomplot.cpp\
        max30102.cpp\
        MaxPlot.cpp \
        MQTTWorker.cpp\
        QRCodeGenerator.cpp \
        MaxDataWorker.cpp

HEADERS  += qcustomplot.h\
            mainwindow.h\
            MaxPlot.h\
            max30102.h \
            MQTTWorker.h \
            QRCodeGenerator.h \
            MaxDataWorker.h
