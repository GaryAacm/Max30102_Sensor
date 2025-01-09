// main.cpp

#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    MainWindow window;
    window.resize(300, 150);
    window.show();

    return app.exec();
}
