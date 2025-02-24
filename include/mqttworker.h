#ifndef MQTTWORKER_H
#define MQTTWORKER_H

#include <QObject>
#include <MQTTClient.h>
#include <QString>
#include <QJsonObject>
#include <QJsonDocument>

class MQTTWorker : public QObject
{
    Q_OBJECT
public:
    MQTTWorker(const char *address, const char *clientId, const char *topic, int qos, long timeout, QObject *parent = nullptr);
    ~MQTTWorker();

public slots:
    void publishMessage(const QString &message);
    void stop();

signals:
    void finished();

private:
    MQTTClient client;
    QString ADDRESS;
    QString CLIENTID;
    QString TOPIC;
    int QOS_LEVEL;
    long TIMEOUT_MS;
    bool running;
};

#endif // MQTTWORKER_H
