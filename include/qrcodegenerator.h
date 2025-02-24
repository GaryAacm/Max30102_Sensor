#ifndef QRCODEGENERATOR_H
#define QRCODEGENERATOR_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <ctime>
#include <random>
#include <algorithm>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <qrencode.h>
#include <png.h>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
using namespace std;

class QRCodeGenerator
{
public:
    QRCodeGenerator();
    ~QRCodeGenerator();

    // Generates and sends user message, returns the user message string
    std::string SendUserMessage();
    string user_message = "";
    std::string user_uuid = "";
    std::string current_time = "";
    std::string device_serial = "";
    std::string numbers_and_letters = "";
    std::string sample_id = " ";

    void Pre_Generate_QRCode();

    void generateAndSaveQRCode();

private:
    // Generates random numbers
    std::string generateRandomNumbers(int count);

    // Generates random letters
    std::string generateRandomLetters(int count);

    // Combines numbers and letters and shuffles the result
    std::string combineNumLetters(int numCount, int letterCount);

    // Retrieves device serial number
    std::string getDeviceSerial();

    // Gets current time in format YYYY-MM-DD-HH-MM-SS
    std::string getCurrentTime();

    // Saves QR code as PNG
    bool saveQrPng(const char *filename, QRcode *qrcode);

    // Callback function for handling CURL data
    static size_t writeCallback(void *contents, size_t size, size_t nmemb, void *userp);

    // Opens the QR code image using a system command
    void openQrImage(const std::string &filename);

   
};

#endif // QRCODEGEN
