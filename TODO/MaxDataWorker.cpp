#include "MaxDataWorker.h"
#include "max30102.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <signal.h>

using namespace std;
using namespace chrono;
int datacount = 0;
void timer_handler(int sig, siginfo_t *si, void *uc)
{

    MAX30102 *data = (MAX30102 *)si->si_value.sival_ptr;
    //data->get_data();
    cout << "data Geting" << endl;
    datacount++;
}

MaxDataWorker::MaxDataWorker(QObject *parent, MAX30102 *sensor)
    : QObject(parent), max30102(sensor)
{
}

MaxDataWorker::~MaxDataWorker()
{
}

void MaxDataWorker::doWork()
{
    // struct sigevent sev;
    // struct sigaction sa;

    // sa.sa_flags = SA_SIGINFO;
    // sa.sa_sigaction = timer_handler;
    // sigemptyset(&sa.sa_mask);
    // sigaction(SIGRTMIN, &sa, NULL);

    // sev.sigev_notify = SIGEV_SIGNAL;
    // sev.sigev_signo = SIGRTMIN;
    // sev.sigev_value.sival_ptr = max30102;

    // timer_t timerid;
    // if (timer_create(CLOCK_REALTIME, &sev, &timerid) == -1)
    // {
    //     perror("timer_create");
    // }

    // struct itimerspec its;
    // its.it_value.tv_sec = 0;
    // its.it_value.tv_nsec = 12500000;
    // its.it_interval.tv_sec = 0;
    // its.it_interval.tv_nsec = 12500000;

    // if (timer_settime(timerid, 0, &its, NULL) == -1)
    // {
    //     perror("timer_settime");
    // }

    //std::this_thread::sleep_for(std::chrono::seconds(5));

    while (1)
    {
        max30102->get_data();
    }

    // timer_delete(timerid);
    // cout << "datacount is :" << datacount << endl;

    // while (datacount < 500)
    // {
    //     max30102->get_data();
    // }

    emit finishRead();
}