#include "MQTTWorker.h"
#include <QDebug>
#include <QStringList>
#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include<iostream>


MQTTWorker::MQTTWorker(const char *address, const char *clientId, const char *topic, int qos, long timeout, QObject *parent)
    : QObject(parent),
      ADDRESS(address),
      CLIENTID(clientId),
      TOPIC(topic),
      QOS_LEVEL(qos),
      TIMEOUT_MS(timeout),
      running(true)
{
    MQTTClient_create(&client, ADDRESS.toUtf8().constData(), CLIENTID.toUtf8().constData(), MQTTCLIENT_PERSISTENCE_NONE, NULL);
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    int rc;
    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
    {
        qDebug() << "无法连接到 MQTT 代理，返回代码：" << rc;
        running = false;
    }
}

MQTTWorker::~MQTTWorker()
{
}

void MQTTWorker::publishMessage(const QString &message)
{
    if (!running)
        return;

    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (doc.isNull())
    {
        qDebug() << "Invalid JSON format";
        return;
    }

    QJsonObject jsonObj = doc.object();

    //Get Array
    QJsonArray channelArray = jsonObj.value("channel").toArray();
    QJsonArray irArray = jsonObj.value("ir").toArray();
    QJsonArray redArray = jsonObj.value("red").toArray();

    if (channelArray.size() != irArray.size() || channelArray.size() != redArray.size())
    {
        qDebug() << "Data size mismatch between channel, ir, and red arrays";
        return;
    }

    for (int i = 0; i < channelArray.size(); ++i)
    {
        int channel = channelArray[i].toInt();
        int ir = irArray[i].toInt();
        int red = redArray[i].toInt();

        qDebug() << "Channel:" << channel << "IR:" << ir << "Red:" << red;
    }

    QJsonDocument outputDoc(jsonObj);
    QString jsonString = QString::fromUtf8(outputDoc.toJson(QJsonDocument::Compact));

    qDebug() << "Publishing MQTT message:" << jsonString;

    // 准备 MQTT 消息
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    QByteArray byteArray = jsonString.toUtf8();
    pubmsg.payload = (void *)byteArray.constData();
    pubmsg.payloadlen = byteArray.size();
    pubmsg.qos = QOS_LEVEL;
    pubmsg.retained = 0;
    MQTTClient_deliveryToken token;

    // 发布 JSON 消息
    int rc = MQTTClient_publishMessage(client, TOPIC.toUtf8().constData(), &pubmsg, &token);
    if (rc != MQTTCLIENT_SUCCESS)
    {
        qDebug() << "发布消息失败，返回代码：" << rc;
        return;
    }
}

void MQTTWorker::stop()
{
    running = false;
    emit finished();
    int rc = MQTTClient_disconnect(client, 1000);
    printf("Closing MQTT client!");
    if (rc != MQTTCLIENT_SUCCESS)
    {
        qDebug() << "断开连接失败，返回代码：" << rc;
    }
    else
    {
        qDebug() << "成功断开与 MQTT 代理的连接。";
    }
}
