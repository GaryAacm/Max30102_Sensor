#ifndef MAXPLOT_H
#define MAXPLOT_H

#include <QMainWindow>
#include "qcustomplot.h"
#include <QSlider>
#include <QTimer>
#include <QVector>
#include <QList>
#include <QThread>
#include <QGestureEvent>
#include <QPinchGesture>
#include <QTouchEvent>
#include <QLabel>
#include <QDateTime>
#include <QTimer>
#include <QPushButton>
#include "max30102.h"
#include "QRCodeGenerator.h"
#include <ctime>
#include <QTimer>
#include <QDebug>
#include <QObject>
#include <QThread>
#include "MaxDataWorker.h"
#include "MQTTWorker.h"
#include <queue>
#include <QJsonObject>
#include <QJsonDocument>
#include <stack>
#include <cstdio>

#define ADDRESS "mqtt://localhost:1883"
#define CLIENTID "backend-client"
#define TOPIC "sensor/data"
#define QOS 1
#define TIMEOUT 10000L

using namespace std;
const int DATA_NUM = 200005;

class MaxPlot : public QMainWindow
{
    Q_OBJECT

public:
    MaxPlot(QWidget *parent, QRCodeGenerator *qrGenerator);
    ~MaxPlot();

signals:
    void windowClosed();
    void sendMQTTMessage(const QString &message);
    void Finish_ALL();
    void finishRead();
    void Finish_Mqtt();
    void Finish_Plot();

protected:
    bool event(QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

    void pinchTriggered(QPinchGesture *gesture);
    bool gestureEvent(QGestureEvent *event);

private slots:
    void updatePlot();
    void onSliderMoved(int position);
    void onExitButtonClicked();
    void onStartButtonClicked();
    void Read_Data_Thread();
    void Update_Plot_Thread();
    void Mqtt_Thread();
    void Get_Mqtt_Message();

    void onSliderPressed();
    void onSliderReleased();

    void Http_Worker_Start();
    void startTimerIfReady();

    void stop_Plot_Timer();
    void stop_Mqtt_Thread();
    void stop_Read_Thread();
    void setup_timer();
    void start_timer();
    void pause_timer();
    void onMqttFinished();
    void delete_timer();

    void onPlotFinished();

public:
    void Init_GUI_SHOW();
    void Connect_Sensor_POSIX();
    void Setup_Background();

    void setupPlot();
    void setupSlider();
    void setupGestures();
    void handleDataReady(const MaxData &data);
    void Start_To_Read();
    void End_All_Test();
    double Pre_Plot_data();
    void PreWork();

    static void posix_timer_handler(union sigval sv);

    int red_send_data[DATA_NUM], ir_send_data[DATA_NUM], channel_send_id[DATA_NUM];

    QCustomPlot *plot;
    QSlider *slider;
    QTimer *timer = nullptr, *Mqtt_timer = nullptr;
    QTimer *timer_start_ready = nullptr;
    QLabel *logo;

    QVector<double> redData_middle, irData_middle, red0, red1, red2, red3, ir1, ir2, ir3, ir0;
    QVector<double> xData, xData_stop;

    int sampleCount;
    QDateTime startTime;
    int windowSize;
    int datacount = 0;

    QVector<uint8_t> channels;
    QPushButton *exitButton;
    QPushButton *startButton;

    queue<uint32_t> Queue_Plot_Red[8], Queue_Mqtt_Red[8], Queue_Plot_IR[8], Queue_Mqtt_IR[8];

    QPoint lastTouchPos;

    bool isTouching;
    bool isView;
    bool Thread_Running_Plot = true;
    bool Thread_Running_Mqtt = true;
    bool IsReading = true;

    QRCodeGenerator *qr;
    time_t Start_TimeStamp;
    time_t End_TimeStamp;
    uint32_t count_data = 0;

    MaxDataWorker *worker;
    QThread *workerThread = nullptr;
    QThread *timerThread = nullptr;
    QThread *mqttThread = nullptr;
    MQTTWorker *mqttWorker = nullptr;
    MAX30102 *max30102 = nullptr;

    timer_t posixTimer;
    QThread *timerThreaded;

    int countPlot = 0;
    int count_Jump = 0;

    bool Is_Running_MQTT = false;
    bool Not_Exit = true;
};

#endif // MAINWINDOW_H
