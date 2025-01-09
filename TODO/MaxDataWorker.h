#ifndef MAXDATAWORKER_H
#define MAXDATAWORKER_H

#include <QObject>
#include <QVector>
#include "max30102.h"
#include <chrono>
#include <atomic>
#include <signal.h>
#include <ctime>

class MaxDataWorker : public QObject
{
    Q_OBJECT

public:
    explicit MaxDataWorker(QObject *parent,MAX30102 *sensor);
    ~MaxDataWorker();

    void doWork();

signals:
    void finishRead();

public:
    MAX30102 *max30102; 

};

#endif // MAXDATAWORKER_H
