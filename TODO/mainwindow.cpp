#include "mainwindow.h"

#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QProcess>
#include <QUrl>
#include <QMessageBox>
#include <QDebug>
#include <QApplication>
#include <QPainter>
#include <QDir>
#include <iostream>
#include <string>
#include <curl/curl.h>
using namespace std;

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent),
      generateQRButton(new QPushButton("生成二维码")),
      startButton(new QPushButton("开始")),
      exitButton(new QPushButton("退出"))
{
    Setup_Window_Button();
    Setup_Layout();
    Setup_Background();
}

MainWindow::~MainWindow()
{
    if (maxPlotWindow)
    {
        delete maxPlotWindow;
    }
    if (qr)
    {
        delete qr;
    }
    int Init_WQTT = system("pkill mosquitto &");
}

void MainWindow::Setup_Window_Button()
{
    setWindowTitle("皮瓣移植工具");

    int Open_MQTT = system("mosquitto -c /etc/mosquitto/conf.d/mosquitto.conf -v &");
    if (Open_MQTT == -1)
    {
        cerr << "Failed to open MQTT" << endl;
    }

    qr = new QRCodeGenerator();

    generateQRButton->setStyleSheet("background:transparent;color:black;border:none;font-weight:bold;font-size:50px;");
    startButton->setStyleSheet("background:transparent;color:black;border:none;font-weight:bold;font-size:50px;");
    exitButton->setStyleSheet("background:transparent;color:black;border:none;font-size:50px;");

    maxPlotWindow = nullptr;

    generateQRButton->setFixedSize(300, 100);
    startButton->setFixedSize(300, 100);
    exitButton->setFixedSize(300, 100);

    connect(generateQRButton, &QPushButton::clicked, this, &MainWindow::onGenerateQRClicked);
    connect(startButton, &QPushButton::clicked, this, &MainWindow::onStartClicked);
    connect(exitButton, &QPushButton::clicked, this, &MainWindow::onExitClicked);
}

void MainWindow::Setup_Layout()
{
    QVBoxLayout *mainLayout = new QVBoxLayout;
    QVBoxLayout *buttonLayout = new QVBoxLayout;
    buttonLayout->addWidget(generateQRButton);
    buttonLayout->addSpacing(50);
    buttonLayout->addWidget(startButton);
    buttonLayout->addSpacing(50);
    buttonLayout->addWidget(exitButton);

    buttonLayout->setContentsMargins(0, 0, 0, 0);

    QWidget *buttonWidget = new QWidget;
    buttonWidget->setLayout(buttonLayout);

    // right and left
    QHBoxLayout *topLayout = new QHBoxLayout;
    topLayout->addStretch(11);
    topLayout->addWidget(buttonWidget);
    topLayout->addStretch(4);

    // up and down
    mainLayout->addStretch(4);
    mainLayout->addLayout(topLayout);
    mainLayout->addStretch(14);

    setLayout(mainLayout);
}

void MainWindow::Setup_Background()
{
    QString imagePath = "/home/orangepi/Desktop/test/smooth_collect/background.png";
    if (!backgroundPixmap.load(imagePath))
    {
        qDebug() << "Can not load the background picture ,please check the path : " << imagePath;
    }

    showFullScreen();
}

void MainWindow::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    if (!backgroundPixmap.isNull())
    {
        painter.drawPixmap(0, 0, width(), height(), backgroundPixmap);
    }
    QWidget::paintEvent(event);
}

size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string *)userp)->append((char *)contents, size * nmemb);
    return size * nmemb;
}

void MainWindow::sendExitMessage()
{
    cout << "Exit out" << endl;
}

void MainWindow::onGenerateQRClicked()
{
    User_Message = qr->generateAndSendUserMessage();
}

void MainWindow::onStartClicked()
{
    if (!maxPlotWindow)
    {
        maxPlotWindow = new MaxPlot(nullptr, qr);
        connect(maxPlotWindow, &MaxPlot::windowClosed, this, [this]()
                { maxPlotWindow = nullptr; });

        maxPlotWindow->show();
    }
    else
    {
        maxPlotWindow->show();
        maxPlotWindow->raise();
        maxPlotWindow->activateWindow();
    }
}

void MainWindow::onExitClicked()
{
    sendExitMessage();
    close();
}
