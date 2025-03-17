#include "MaxPlot.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGestureEvent>
#include <QPinchGesture>
#include <QDebug>
#include <QPixmap>
#include <QDateTime>
#include <MQTTClient.h>
#include <iostream>
#include <QMessageBox>
#include <QTimer>
#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrent>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <QTimer>
#include <QDebug>
#include <QObject>
#include <QThread>
#include <QStringList>
#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <zlib.h>
#include<thread>

using namespace std;
using namespace nlohmann;
using json = nlohmann::json;

MaxPlot::MaxPlot(QWidget *parent, QRCodeGenerator *qrGenerator)
    : QMainWindow(parent), plot(new QCustomPlot(this)), logo(new QLabel(this)),
      sampleCount(0), windowSize(30), isTouching(false), qr(qrGenerator)
{
    Init_GUI_SHOW();
}

MaxPlot::~MaxPlot()
{
    stop_Plot_Timer();
    stop_Mqtt_Thread();
}

void MaxPlot::Init_GUI_SHOW()
{
    Setup_Background();

    setup_timer();

    Connect_Sensor_POSIX();

    Start_To_Read();

    End_All_Test();
}

void MaxPlot::Connect_Sensor_POSIX()
{
    connect(max30102, &MAX30102::dataReady, this, &MaxPlot::handleDataReady);
    connect(max30102, &MAX30102::finishRead, this, &MaxPlot::Http_Worker_Start);
    connect(max30102, &MAX30102::finishRead, this, &MaxPlot::pause_timer);

    qRegisterMetaType<MaxData>("MaxData");
}

void MaxPlot::setup_timer()
{
    if (!max30102)
    {
        cout << "Creat max30102 ptr" << endl;
        max30102 = new MAX30102("/dev/i2c-4");
    }

    //max30102->Init_Sensor();

    struct sigevent sev;
    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_notify_function = posix_timer_handler;
    sev.sigev_value.sival_ptr = max30102;

    if (timer_create(CLOCK_REALTIME, &sev, &posixTimer) == -1)
    {
        std::cerr << "创建 POSIX 定时器失败" << std::endl;
    }
    else
    {
        std::cout << "POSIX 定时器创建成功" << std::endl;
    }
}

void MaxPlot::start_timer()
{

    struct itimerspec its;
    its.it_value.tv_sec = 0;
    its.it_value.tv_nsec = 12500000;
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 12500000;

    if (timer_settime(posixTimer, 0, &its, NULL) == -1)
    {
        std::cerr << "设置 POSIX 定时器失败" << std::endl;
    }
    else
    {
        std::cout << "POSIX 定时器启动成功" << std::endl;
    }
}

void MaxPlot::pause_timer()
{
    struct itimerspec its;
    its.it_value.tv_sec = 0;
    its.it_value.tv_nsec = 0;
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 0;

    if (timer_settime(posixTimer, 0, &its, NULL) == -1)
    {
        std::cerr << "停止 POSIX 定时器失败" << std::endl;
    }
    else
    {
        std::cout << "POSIX 定时器已暂停" << std::endl;
    }
    IsReading = false;
}

void MaxPlot::posix_timer_handler(union sigval sv)
{
    MAX30102 *data = static_cast<MAX30102 *>(sv.sival_ptr);

    // 尝试将 is_reading 从 false 设置为 true
    bool expected = false;
    if (data->is_reading.compare_exchange_strong(expected, true))
    {

        data->get_data();

        data->datacount++;
        if (data->datacount >= 3600)
        {
            std::cout << "data is up to full" << std::endl;

            cout << "Data count is " << data->datacount << endl;
            data->datacount = 0;
            emit data->finishRead();
            data->Quit();

            cout << "Jump is:" << data->count_Jump << endl;

            data->count_Jump = 0;
        }

        data->is_reading = false;
    }
    else
    {

        std::cerr << "上一次读取未完成，跳过此次读取" << std::endl;
        data->count_Jump++;
        data->datacount++;
    }
}

void MaxPlot::Start_To_Read()
{
    connect(startButton, &QPushButton::clicked, this, &MaxPlot::onStartButtonClicked);
}

void MaxPlot::startTimerIfReady()
{

    if (!Thread_Running_Plot && !Thread_Running_Mqtt)
    {
        cout << "Beginning to stop and begin after 255 s" << endl;

        stop_Mqtt_Thread();

        cout << "Now start the control" << endl;

        if (timer_start_ready == nullptr)
        {
            timer_start_ready = new QTimer(this);
            timer_start_ready->setInterval(255000);
            connect(timer_start_ready, &QTimer::timeout, [this]()
                    {
                        onStartButtonClicked();
                        timer_start_ready->deleteLater(); // 删除定时器
                        timer_start_ready = nullptr;      // 清空指针
                    });
        }
        timer_start_ready->start();
    }
}

void MaxPlot::delete_timer()
{
    // 删除定时器
    if (timer_delete(posixTimer) == -1)
    {
        std::cerr << "删除 POSIX 定时器失败" << std::endl;
    }
    else
    {
        std::cout << "POSIX 定时器已删除" << std::endl;
    }
}

void MaxPlot::End_All_Test()
{
    connect(exitButton, &QPushButton::clicked, this, &MaxPlot::onExitButtonClicked);
}

void MaxPlot::Setup_Background()
{
    this->setStyleSheet("background-color:black;");

    QWidget *centralWidget = new QWidget(this);
    centralWidget->setStyleSheet("background-color:black");
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    QPixmap logoPixmap("/home/orangepi/Desktop/zjj/Qt_use/logo.png");
    QPixmap scaledLogo = logoPixmap.scaled(300, 100, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    logo->setPixmap(scaledLogo);
    logo->setAlignment(Qt::AlignCenter);

    logo->setStyleSheet("background-color:black;");
    logo->setFixedHeight(120);

    setupPlot();
    setupSlider();
    setupGestures();

    // Get start time stamp
    Start_TimeStamp = time(nullptr);

    startButton = new QPushButton("Begin", this);
    startButton->setFixedSize(100, 40);
    startButton->setStyleSheet("background-color:white;");

    exitButton = new QPushButton("Exit", this);
    exitButton->setFixedSize(100, 40);
    exitButton->setStyleSheet("background-color:white;");

    QHBoxLayout *topLayout = new QHBoxLayout();
    topLayout->addWidget(exitButton);
    topLayout->addWidget(startButton);

    QHBoxLayout *bottomLayout = new QHBoxLayout();
    bottomLayout->addWidget(slider);

    mainLayout->addWidget(logo, 0);
    mainLayout->addWidget(plot, 1);

    mainLayout->addLayout(topLayout);
    mainLayout->addLayout(bottomLayout, 0);

    setCentralWidget(centralWidget);

    startTime = QDateTime::currentDateTime();

    setAttribute(Qt::WA_AcceptTouchEvents);

    showFullScreen();
}

void MaxPlot::PreWork()
{
    IsReading = true;

    startButton->setEnabled(false);
}

void MaxPlot::onStartButtonClicked()
{
    std::system("clear");
    PreWork();
    Read_Data_Thread();
    Update_Plot_Thread();
    Mqtt_Thread();
}

void MaxPlot::Read_Data_Thread()
{
    max30102->Init_Sensor();
    start_timer();
}

void MaxPlot::stop_Read_Thread()
{

    if (workerThread)
    {
        workerThread->quit();
        workerThread->wait();
        delete workerThread;
        workerThread = nullptr;
    }
}

void MaxPlot::Update_Plot_Thread()
{
    Thread_Running_Plot = true;
    if (!timer)
    {
        timer = new QTimer(this);
        timer->setTimerType(Qt::PreciseTimer);
        cout << "Creat timer update plot" << endl;
    }
    connect(timer, &QTimer::timeout, this, &MaxPlot::updatePlot);
    timer->start(50);
}



void MaxPlot::Mqtt_Thread()
{

    Thread_Running_Mqtt = true;
    Is_Running_MQTT = true;
    if (!Mqtt_timer)
    {
        Mqtt_timer = new QTimer();
        cout << "Creat Mqtt timer " << endl;
    }

    if (!mqttThread)
    {
        mqttThread = new QThread();
        cout << "Creat Mqtt thread " << endl;
    }

    if (mqttWorker == nullptr)
    {
        mqttWorker = new MQTTWorker(ADDRESS, CLIENTID, TOPIC, QOS, TIMEOUT);
        mqttWorker->moveToThread(mqttThread);
        cout << "Creat Mqtt worker " << endl;
    }

    if (!connect(this, &MaxPlot::sendMQTTMessage, mqttWorker, &MQTTWorker::publishMessage, Qt::UniqueConnection))
    {
        qWarning("Signal 'sendMQTTMessage' already connected to 'publishMessage' in MQTTWorker.");
    }

    mqttThread->start();

    Mqtt_timer->setInterval(33);

    if (!connect(Mqtt_timer, &QTimer::timeout, this, &MaxPlot::Get_Mqtt_Message, Qt::UniqueConnection))
    {
        qWarning("Signal 'timeout' from Mqtt_timer already connected to 'Get_Mqtt_Message' in MaxPlot.");
    }

    Mqtt_timer->start();
}

void MaxPlot::stop_Plot_Timer()
{
    if (timer && timer->isActive())
    {
        timer->stop();
        cout << "Already stop plot timer" << endl;
    }
}

void MaxPlot::stop_Mqtt_Thread()
{
    if (Mqtt_timer && Mqtt_timer->isActive())
    {
        Mqtt_timer->stop();
        cout << "stop mqtt timer" << endl;
    }

    if (Mqtt_timer)
    {
        delete Mqtt_timer;
        Mqtt_timer = nullptr;
        cout << "delete mqtt timer" << endl;
    }

    if (mqttWorker)
    {
        mqttWorker->stop();
        delete mqttWorker;
        mqttWorker = nullptr;
        cout << "delet mqtt worker" << endl;
    }

    mqttThread->quit();
    mqttThread->wait();
    if (!mqttThread)
    {
        delete mqttThread;
        mqttThread = nullptr;
        cout << "delete mqtt thread" << endl;
    }
}

void MaxPlot::onMqttFinished()
{
    Thread_Running_Mqtt = false;
    for (int i = 0; i < 8; i++)
    {
        while (!Queue_Mqtt_Red[i].empty())
            Queue_Mqtt_Red[i].pop();

        while (!Queue_Mqtt_IR[i].empty())
            Queue_Mqtt_IR[i].pop();
    }
    startTimerIfReady();
}

void MaxPlot::onPlotFinished()
{
    Thread_Running_Plot = false;
    for (int i = 0; i < 8; i++)
    {
        while (!Queue_Plot_Red[i].empty())
            Queue_Plot_Red[i].pop();

        while (!Queue_Plot_IR[i].empty())
            Queue_Plot_IR[i].pop();
    }

    startTimerIfReady();
}

// 压缩函数，使用zlib的compress函数
bool compressString(const std::string &str, std::vector<unsigned char> &compressedData)
{
    uLongf compressedSize = compressBound(str.size());
    compressedData.resize(compressedSize);

    int res = compress(compressedData.data(), &compressedSize, reinterpret_cast<const Bytef *>(str.data()), str.size());
    if (res != Z_OK)
    {
        std::cerr << "压缩失败，zlib错误码: " << res << std::endl;
        return false;
    }

    compressedData.resize(compressedSize);
    return true;
}

void Send_Message(const std::string &Start_Unix, const int *Channel_ID, const int *Red_Data, const int *IR_Data, const std::string &sample_id, const std::string &uuid, const int fre)
{
    CURL *curl = curl_easy_init();

    if (curl)
    {
        int Data_Size = 45 * 640;
        json jsonData;
        jsonData["Start_Unix"] = Start_Unix;

        jsonData["channel_id"] = std::vector<int>(Channel_ID, Channel_ID + Data_Size);
        jsonData["data"]["ir"] = std::vector<int>(IR_Data, IR_Data + Data_Size);
        jsonData["data"]["reds"] = std::vector<int>(Red_Data, Red_Data + Data_Size);

        jsonData["sample_id"] = sample_id;
        jsonData["user_uuid"] = uuid;

        jsonData["frequency"] = fre;

        string jsonString = jsonData.dump();

        std::vector<unsigned char> compressedData;

        if (!compressString(jsonString, compressedData))
        {
            std::cerr << "压缩 JSON 数据失败。" << std::endl;
            return;
        }

        std::string url = "http://sp.grifcc.top:8080/collect/data";

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1L);

        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/octet-stream");
        headers = curl_slist_append(headers, "Content-Encoding: deflate");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // POST DATA
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, compressedData.data());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, compressedData.size());

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK)
            std::cerr << "curl_easy_perform() 失败: " << curl_easy_strerror(res) << std::endl;

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
    else
    {
        std::cerr << "curl_easy_init() 失败" << std::endl;
    }
}

void MaxPlot::Http_Worker_Start()
{

    // Init HTTP
    CURLcode res = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (res != CURLE_OK)
    {
        std::cout << "curl_global_init() 失败: " << curl_easy_strerror(res) << std::endl;
    }

    // Send ID
    string userdata = qr->user_message;
    size_t pos = userdata.find(',');
    string sample_id = userdata.substr(0, pos);
    string uuid = userdata.substr(pos + 1);

    // Send Time
    long ts_start = static_cast<long>(Start_TimeStamp);
    string Start_string = to_string(ts_start);

    std::thread sendThread(Send_Message, Start_string, channel_send_id, red_send_data, ir_send_data, sample_id, uuid, 80);
    sendThread.detach();
    curl_global_cleanup();

    onMqttFinished();
    onPlotFinished();
    count_data = 0;
}

void MaxPlot::Get_Mqtt_Message()
{
    // To do ke cha jie
    uint32_t red_temp[8] = {0}, ir_temp[8] = {0}, channel_temp[8] = {0};

    for (int i = 0; i < 8; i++)
    {
        if (Queue_Mqtt_Red[i].size() == 0 || Queue_Mqtt_IR[i].size() == 0)
            continue;
        red_temp[i] = Queue_Mqtt_Red[i].back();

        ir_temp[i] = Queue_Mqtt_IR[i].back();
        channel_temp[i] = i;
    }

    QJsonArray redTempArray;
    QJsonArray irTempArray;
    QJsonArray channelArray;

    for (int i = 0; i < 8; i++)
    {
        redTempArray.append(static_cast<int>(red_temp[i]));
        irTempArray.append(static_cast<int>(ir_temp[i]));
        channelArray.append(static_cast<int>(channel_temp[i]));
    }

    // 构建 QJsonObject
    QJsonObject payloadObj;
    payloadObj.insert("channel", channelArray);
    payloadObj.insert("ir", irTempArray);
    payloadObj.insert("red", redTempArray);

    // 转换为 JSON 字符串
    QJsonDocument doc(payloadObj);
    QString payload = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));

    emit sendMQTTMessage(payload);
}

void MaxPlot::onExitButtonClicked()
{
    exitButton->setEnabled(false);

    pause_timer();
    if (Is_Running_MQTT)
    {
        stop_Mqtt_Thread();
    }
    stop_Plot_Timer();
    delete_timer();

    if (timer_start_ready != nullptr)
    {
        timer_start_ready->stop();        // 停止定时器
        timer_start_ready->deleteLater(); // 删除定时器
        timer_start_ready = nullptr;      // 清空指针
        cout << "Timer stopped after exit." << endl;
    }

    close();
}

void MaxPlot::handleDataReady(const MaxData &data)
{
    // cout<<"Deal data"<<endl;
    uint32_t temp_red = 0, temp_ir = 0;
    for (int i = 0; i < 8; i++)
    {
        Queue_Mqtt_Red[i].push(data.redData[i]);
        Queue_Mqtt_IR[i].push(data.irData[i]);

        Queue_Plot_Red[i].push(data.redData[i]);
        Queue_Plot_IR[i].push(data.irData[i]);

        channel_send_id[count_data] = i;
        red_send_data[count_data] = data.redData[i];
        ir_send_data[count_data++] = data.irData[i];
    }
    // cout<<"count is"<<countPlot++<<endl;
}

void MaxPlot::setupPlot()
{
    plot->addGraph();

    plot->graph(0)->setPen(QPen(Qt::red));
    plot->graph(0)->setLineStyle(QCPGraph::lsLine);

    plot->graph(0)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssNone));

    plot->addGraph();

    plot->graph(1)->setPen(QPen(Qt::green));
    plot->graph(1)->setLineStyle(QCPGraph::lsLine);
    plot->graph(1)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssNone));

    QList<QColor> colors = {Qt::gray, Qt::blue, Qt::gray, Qt::blue, Qt::cyan, Qt::darkBlue, Qt::yellow, Qt::darkYellow, Qt::magenta, Qt::darkGreen};
    for (int i = 2; i < 10; ++i)
    {
        plot->addGraph();

        plot->graph(i)->setPen(QPen(colors[i % colors.size()]));

        plot->graph(i)->setLineStyle(QCPGraph::lsLine);
        plot->graph(i)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssNone));
    }

    plot->xAxis->setLabel("Time (s)");
    plot->yAxis->setLabel("Amplitude");

    plot->xAxis->setRange(0, windowSize);
    plot->yAxis->setRange(0, 200000);

    plot->legend->setVisible(false);

    plot->setBackground(Qt::black);
    plot->xAxis->setBasePen(QPen(Qt::white));
    plot->yAxis->setBasePen(QPen(Qt::white));
    plot->xAxis->setTickPen(QPen(Qt::white));
    plot->yAxis->setTickPen(QPen(Qt::white));
    plot->xAxis->setSubTickPen(QPen(Qt::white));
    plot->yAxis->setSubTickPen(QPen(Qt::white));
    plot->xAxis->setLabelColor(Qt::white);
    plot->yAxis->setLabelColor(Qt::white);
    plot->xAxis->setTickLabelColor(Qt::white);
    plot->yAxis->setTickLabelColor(Qt::white);
}

void MaxPlot::setupGestures()
{
    grabGesture(Qt::PinchGesture);
}

bool MaxPlot::event(QEvent *event)
{
    if (event->type() == QEvent::Gesture)
    {
        return gestureEvent(static_cast<QGestureEvent *>(event));
    }

    return QMainWindow::event(event);
}

bool MaxPlot::gestureEvent(QGestureEvent *event)
{
    if (QGesture *pinch = event->gesture(Qt::PinchGesture))
    {
        pinchTriggered(static_cast<QPinchGesture *>(pinch));
    }
    return true;
}

void MaxPlot::setupSlider()
{
    slider = new QSlider(Qt::Horizontal, this);
    slider->setRange(0, 0);
    slider->setEnabled(false);
    slider->setStyleSheet("background-color:white;");
    connect(slider, &QSlider::valueChanged, this, &MaxPlot::onSliderMoved);
    connect(slider, &QSlider::sliderPressed, this, &MaxPlot::onSliderPressed);
    connect(slider, &QSlider::sliderReleased, this, &MaxPlot::onSliderReleased);
    isView = false;
}

void MaxPlot::onSliderPressed()
{
    isView = true;
}

void MaxPlot::onSliderReleased()
{
    int sliderMax = slider->maximum();
    int position = slider->value();
    isView = false;
}

void MaxPlot::onSliderMoved(int position)
{
    if (isView)
    {
        plot->xAxis->setRange(position, position + windowSize);
        plot->replot();
    }
}

void MaxPlot::pinchTriggered(QPinchGesture *gesture)
{
    QPinchGesture::ChangeFlags changeFlags = gesture->changeFlags();
    if (changeFlags & QPinchGesture::ScaleFactorChanged)
    {
        qreal scaleFactor = gesture->scaleFactor();

        plot->xAxis->scaleRange(scaleFactor, plot->xAxis->range().center());
        plot->yAxis->scaleRange(scaleFactor, plot->yAxis->range().center());

        plot->replot();
    }
}

double MaxPlot::Pre_Plot_data()
{
    uint32_t temp_red = 0, temp_ir = 0, mean_count = 0;
    uint32_t reddata[8], irdata[8], red = 0, ir = 0, reddata_past[8] = {0}, irdata_past[8] = {0};

    double elapsedTime = startTime.msecsTo(QDateTime::currentDateTime()) / 1000.0;

    for (int i = 0; i < 8; i++)
    {
        if (Queue_Plot_Red[i].size() == 0 || Queue_Plot_IR[i].size() == 0)
            continue;

        red = Queue_Plot_Red[i].back();

        ir = Queue_Plot_IR[i].back();

        if (i == 0 || i == 2 || i == 4 || i == 6)
        {
            temp_red += red;
            temp_ir += ir;
            mean_count++;
        }
        else
        {
            reddata[i] = red;
            irdata[i] = ir;
        }
    }
    temp_red /= mean_count;
    temp_ir /= mean_count;

    redData_middle.append(static_cast<double>(temp_red));
    irData_middle.append(static_cast<double>(temp_ir));

    red0.append(static_cast<double>(reddata[1]));
    ir0.append(static_cast<double>(irdata[1]));

    red1.append(static_cast<double>(reddata[3]));
    ir1.append(static_cast<double>(irdata[3]));

    red2.append(static_cast<double>(reddata[5]));
    ir2.append(static_cast<double>(irdata[5]));

    red3.append(static_cast<double>(reddata[7]));
    ir3.append(static_cast<double>(irdata[7]));
    xData.append(elapsedTime);

    //cout << "count is" << countPlot++ << endl;

    return elapsedTime;
}

void MaxPlot::updatePlot()
{

    if (IsReading)
    {
        double elapsedTime = Pre_Plot_data();
        plot->graph(0)->setData(xData, redData_middle);
        plot->graph(1)->setData(xData, irData_middle);

        plot->graph(2)->setData(xData, red0);
        plot->graph(3)->setData(xData, ir0);

        plot->graph(4)->setData(xData, red1);
        plot->graph(5)->setData(xData, ir1);

        plot->graph(6)->setData(xData, red2);
        plot->graph(7)->setData(xData, ir2);

        plot->graph(8)->setData(xData, red3);
        plot->graph(9)->setData(xData, ir3);
    }
    else
    {
        double elapsedTime = startTime.msecsTo(QDateTime::currentDateTime()) / 1000.0;

        redData_middle.append(static_cast<double>(0));
        irData_middle.append(static_cast<double>(0));

        red0.append(static_cast<double>(0));
        ir0.append(static_cast<double>(0));

        red1.append(static_cast<double>(0));
        ir1.append(static_cast<double>(0));

        red2.append(static_cast<double>(0));
        ir2.append(static_cast<double>(0));

        red3.append(static_cast<double>(0));
        ir3.append(static_cast<double>(0));
        xData.append(elapsedTime);
        countPlot = 0;
    }

    sampleCount++;

    double elapsedTime = startTime.msecsTo(QDateTime::currentDateTime()) / 1000.0;

    if (elapsedTime <= windowSize)
    {
        plot->xAxis->setRange(0, windowSize);
        slider->setRange(0, windowSize);
        slider->setEnabled(false);
    }
    else
    {
        if (!slider->isEnabled())
        {
            slider->setEnabled(true);
        }
        double maxSliderValue = elapsedTime - windowSize;

        if (!slider->isSliderDown())
        {
            slider->setRange(0, static_cast<int>(elapsedTime - windowSize));

            if (!isView)
            {

                slider->setValue(static_cast<int>(maxSliderValue));
                plot->xAxis->setRange(elapsedTime - windowSize, elapsedTime);
            }
            plot->xAxis->setRange(elapsedTime - windowSize, elapsedTime);
        }
    }
    plot->replot();
}

void MaxPlot::closeEvent(QCloseEvent *event)
{
    emit windowClosed();
    QMainWindow::closeEvent(event);
}
