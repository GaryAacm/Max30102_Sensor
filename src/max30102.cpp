#include "max30102.h"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <QString>
#include <QByteArray>
#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>
#include <chrono>
using namespace std;

MAX30102::MAX30102(const char *device, uint8_t tcaAddress, uint8_t maxAddress)
    : device(device), tcaAddress(tcaAddress), maxAddress(maxAddress)
{
    fd = init_i2c(device, TCA9548A_ADDR);
    scanf_channel();
}

MAX30102::~MAX30102()
{
}

void MAX30102::Init_Sensor()
{
    fd = init_i2c(device, TCA9548A_ADDR);
    // scanf_channel();
    Init_channel_sensor();
}

void MAX30102::Quit()
{
    close(fd);
}

void MAX30102::writeRegister(uint8_t max_fd, uint8_t reg, uint8_t add)
{
    uint8_t config[2] = {reg, add};
    write(max_fd, config, 2);
}

void MAX30102::max30102_init(int max_fd)
{
    writeRegister(max_fd, REG_MODE_CONFIG, 0x40);
    writeRegister(max_fd, REG_FIFO_WR_PTR, 0x00);
    writeRegister(max_fd, REG_OVF_COUNTER, 0x00);
    writeRegister(max_fd, REG_FIFO_RD_PTR, 0x00);
    writeRegister(max_fd, REG_INTR_ENABLE_1, 0xE0);

    writeRegister(max_fd, REG_INTR_ENABLE_2, 0x00);
    writeRegister(max_fd, REG_FIFO_CONFIG, 0x0F);
    writeRegister(max_fd, REG_MODE_CONFIG, 0x03);
    writeRegister(max_fd, REG_SPO2_CONFIG, 0x27);
    writeRegister(max_fd, REG_RED_LED, 0x24);
    writeRegister(max_fd, REG_IR_LED, 0x24);
    writeRegister(max_fd, REG_PILOT_PA, 0x7F);
}

// 自定义删除器
struct CloseDeleter
{
    void operator()(int *fd) const
    {
        if (fd)
        {
            close(*fd);
        }
    }
};

void MAX30102::scanf_channel()
{
    for (int i = 0; i < 8; i++)
    {
        char command[128];
        snprintf(command, sizeof(command), "i2cset -y 4 0x%02x 0x00 0x%02x", TCA9548A_ADDR, 1 << i);

        if (system(command) != 0)
        {
            continue;
        }

        char detect_command[128];
        snprintf(detect_command, sizeof(detect_command), "i2cdetect -y 4 | grep -q '57'");

        if (system(detect_command) == 0)
        {
            enable_channels[count_channel++] = i;
        }
        else
        {
            std::cerr << "No device found at address 0x57 on channel " << std::endl;
            continue;
        }
    }
    cout << "count_channel is:" << count_channel << endl;
    for (int i = 0; i < count_channel; i++)
    {
        std::cout << i << "channel : " << enable_channels[i] << std::endl;
    }
}

void MAX30102::Init_channel_sensor()
{
    for (int i = 0; i <= count_channel; i++)
    {
        int channel = i;
        int use = 1 << enable_channels[i];
        int check = write(fd, &use, 1);
        if (check != 1)
            continue;

        std::unique_ptr<int, CloseDeleter> max30102_fd_ptr(
            new int(init_i2c(device, MAX30102_ADDR)));

        if (*max30102_fd_ptr == -1)
        {
            perror("Failed to setup MAX30102");
            continue;
        }

        max30102_init(*max30102_fd_ptr);
    }
}

void MAX30102::read_fifo(uint32_t *red_led, uint32_t *ir_led, int fd)
{
    uint8_t reg = REG_FIFO_DATA;
    uint8_t reg_data[6];

    if (write(fd, &reg, 1) != 1)
    {
        return;
    }
    if (read(fd, reg_data, 6) != 6)
    {
        return;
    }
    *red_led = ((reg_data[0] & 0x03) << 16) | (reg_data[1] << 8) | reg_data[2];
    *ir_led = ((reg_data[3] & 0x03) << 16) | (reg_data[4] << 8) | reg_data[5];
}

int MAX30102::init_i2c(const char *device, int addr)
{
    int temp_fd = open(device, O_RDWR);
    if (temp_fd == -1)
    {
        perror("Failed to open I2C device");
        return -1;
    }
    if (ioctl(temp_fd, I2C_SLAVE, addr) < 0)
    {
        perror("Failed to set I2C address");
        close(temp_fd);
        return -1;
    }

    return temp_fd;
}

void MAX30102::reset_data()
{
    for (int i = 0; i <= count_channel; i++)
    {
        data.channel_id[i] = i;
        data.redData[i] = 0;
        data.irData[i] = 0;
    }

    emit dataReady(data);
}

void MAX30102::get_data()
{
    uint32_t red_temp, ir_temp;

    for (int i = 0; i < count_channel; i++)
    {
        int use_channel = 1 << enable_channels[i];
        cout << i << "Now is" << enable_channels[i] << endl;
        int check = write(fd, &use_channel, 1);
        if (check != 1)
            continue;

        std::unique_ptr<int, CloseDeleter> max30102_fd_ptr(
            new int(init_i2c(device, MAX30102_ADDR)));

        if (*max30102_fd_ptr == -1)
        {
            perror("Failed to setup MAX30102");
            continue;
        }

        read_fifo(&red_temp, &ir_temp, *max30102_fd_ptr);

        printf("Reading data is:%d %d %d\n", enable_channels[i], red_temp, ir_temp);

        data.redData[enable_channels[i]] = red_temp;
        data.irData[enable_channels[i]] = ir_temp;
    }
    // cout << "Check_data is" << check_count++ << endl;

    emit dataReady(data);
}
