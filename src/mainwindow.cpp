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
      generateQRButton(new QPushButton("开始")),
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
    exitButton->setStyleSheet("background:transparent;color:black;border:none;font-size:50px;");

    maxPlotWindow = nullptr;

    generateQRButton->setFixedSize(300, 100);

    exitButton->setFixedSize(300, 100);

    connect(generateQRButton, &QPushButton::clicked, this, &MainWindow::onGenerateQRClicked);
    connect(exitButton, &QPushButton::clicked, this, &MainWindow::onExitClicked);
}

void MainWindow::Setup_Layout()
{
    QVBoxLayout *mainLayout = new QVBoxLayout;
    QVBoxLayout *buttonLayout = new QVBoxLayout;
    buttonLayout->addWidget(generateQRButton);
    buttonLayout->addSpacing(50);

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
    qr->Pre_Generate_QRCode();
    qr->generateAndSaveQRCode();
    QString qrImagePath = "/home/orangepi/Desktop/test/smooth_collect/QRcode.png";

    QDialog *qrDialog = new QDialog(this);
    qrDialog->setWindowTitle("二维码");
    QLabel *qrImageLabel = new QLabel(qrDialog);
    QPixmap qrPixmap(qrImagePath);
    qrImageLabel->setPixmap(qrPixmap);
    qrImageLabel->setAlignment(Qt::AlignCenter);

    QLabel *statusLabel = new QLabel("请扫码，等待检测中...", qrDialog);
    statusLabel->setAlignment(Qt::AlignCenter);

    QPushButton *closeButton = new QPushButton("关闭", qrDialog);
    closeButton->setStyleSheet("background-color: red; color: white; font-size: 20px;");

    // 创建定时器
    QTimer *checkTimer = new QTimer(qrDialog);
    QTimer *timeoutTimer = new QTimer(qrDialog);

    // 检测函数的计数器
    int checkCount = 0;
    const int CHECK_INTERVAL = 1000;
    const int TIMEOUT_CHECK = 60000;

    // 设置检测定时器
    connect(checkTimer, &QTimer::timeout, [=]() mutable
            {
        checkCount++;
        
        bool result = checkStatus();
        
        if (result) {
            statusLabel->setText("检测成功，即将进入测量");
            statusLabel->setStyleSheet("color: green; font-size: 18px;");
            checkTimer->stop();
            timeoutTimer->stop();
            Start_Get_UUID();
            qrDialog->accept();
        } else {
            statusLabel->setText(QString("等待检测中...（%1秒）").arg(checkCount));
        } });

    // 设置超时定时器
    connect(timeoutTimer, &QTimer::timeout, [=]()
            {
        statusLabel->setText("未检测到扫码，请重新点击开始");
        statusLabel->setStyleSheet("color: red; font-size: 18px;");
        checkTimer->stop();
        qrDialog->accept(); });

    connect(closeButton, &QPushButton::clicked, [=]()
            {
        checkTimer->stop();
        timeoutTimer->stop();
        delete checkTimer;
        delete timeoutTimer;
        qrDialog->accept(); });

    // 启动定时器
    checkTimer->start(CHECK_INTERVAL);
    timeoutTimer->start(TIMEOUT_CHECK);

    QVBoxLayout *layout = new QVBoxLayout(qrDialog);
    layout->addWidget(qrImageLabel);
    layout->addWidget(statusLabel);
    layout->addWidget(closeButton);
    qrDialog->setLayout(layout);
    qrDialog->resize(400, 500);

    qrDialog->exec();
}

bool MainWindow::checkStatus()
{
    string Check_Result = qr->SendUserMessage();

    cout << "Check Result is " << Check_Result << endl;

    if (Check_Result != "")
        return true;

    return false;
}

void MainWindow::Start_Get_UUID()
{
    onStartClicked();
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