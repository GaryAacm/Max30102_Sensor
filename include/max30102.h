#ifndef MAX30102_H
#define MAX30102_H

#include <fcntl.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <QCoreApplication>
#include <QTimer>
#include <QDebug>
#include <QObject>
#include <QThread>

using namespace std;

// I2C 地址
#define MAX30102_ADDR 0x57
#define TCA9548A_ADDR 0x77

// MAX30102 寄存器
#define REG_INTR_STATUS_1 0x00
#define REG_INTR_STATUS_2 0x01
#define REG_INTR_ENABLE_1 0x02
#define REG_INTR_ENABLE_2 0x03
#define REG_FIFO_WR_PTR 0x04
#define REG_OVF_COUNTER 0x05
#define REG_FIFO_RD_PTR 0x06
#define REG_FIFO_DATA 0x07
#define REG_FIFO_CONFIG 0x08
#define REG_MODE_CONFIG 0x09
#define REG_SPO2_CONFIG 0x0A
#define REG_RED_LED 0x0C
#define REG_IR_LED 0x0D
#define REG_PILOT_PA 0x10
#define REG_MULTI_LED_CTRL1 0x11
#define REG_MULTI_LED_CTRL2 0x12

struct MaxData
{
    uint32_t redData[8]={0}, irData[8]={0}, channel_id[8]={0,1,2,3,4,5,6,7};
};

Q_DECLARE_METATYPE(MaxData)

class MAX30102 : public QObject
{
    Q_OBJECT
public:
   
    MAX30102(const char *device, uint8_t tcaAddress = TCA9548A_ADDR, uint8_t maxAddress = MAX30102_ADDR);

    ~MAX30102();

    void max30102_init(int fd);

    void writeRegister(uint8_t fd, uint8_t reg, uint8_t add);

    void read_fifo(uint32_t *red_led, uint32_t *ir_led, int fd);

    int init_i2c(const char *device, int addr);

    void scanf_channel();

    void Init_channel_sensor();

    void get_data();

    void Quit();

    void Init_Sensor();

    void reset_data();

    std::atomic<bool> is_reading{false};

    MaxData data;
    int datacount = 0;
    int count_Jump = 0;
    int check_count = 0;

signals:
    void dataReady(const MaxData &data);
    void finishRead();

private:
    const char *device;
    uint8_t tcaAddress;
    uint8_t maxAddress;
    int fd;
    int max30102_fd;
    int enable_channels[8];
    int count_channel = 0;
    vector<int> max30102_fds;
   

};

#endif
