#include "QRCodeGenerator.h"

QRCodeGenerator::QRCodeGenerator()
{
}

QRCodeGenerator::~QRCodeGenerator()
{
}

std::string QRCodeGenerator::generateRandomNumbers(int count)
{
    std::string numbers;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 9);

    for (int i = 0; i < count; ++i)
    {
        numbers += std::to_string(dis(gen));
    }

    return numbers;
}

std::string QRCodeGenerator::generateRandomLetters(int count)
{
    std::string letters;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 51);

    for (int i = 0; i < count; ++i)
    {
        int idx = dis(gen);
        if (idx < 26)
        {
            letters += 'a' + idx;
        }
        else
        {
            letters += 'A' + (idx - 26);
        }
    }

    return letters;
}

std::string QRCodeGenerator::combineNumLetters(int numCount, int letterCount)
{
    std::string combined = generateRandomNumbers(numCount) + generateRandomLetters(letterCount);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::shuffle(combined.begin(), combined.end(), gen);
    return combined;
}

std::string QRCodeGenerator::getDeviceSerial()
{
    std::ifstream cpuinfo("/proc/cpuinfo");
    if (cpuinfo.is_open())
    {
        std::string line;
        while (std::getline(cpuinfo, line))
        {
            if (line.substr(0, 6) == "Serial")
            {
                std::string serial = line.substr(line.find(":") + 1);
                serial.erase(std::remove(serial.begin(), serial.end(), ' '), serial.end());
                return serial;
            }
        }
    }
    return "000000000";
}

std::string QRCodeGenerator::getCurrentTime()
{
    std::time_t now = std::time(nullptr);
    std::tm tm_struct = *std::localtime(&now);
    char buffer[20];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d-%H-%M-%S", &tm_struct);
    return std::string(buffer);
}

bool QRCodeGenerator::saveQrPng(const char *filename, QRcode *qrcode)
{
    if (!qrcode)
    {
        return false;
    }

    FILE *fp = fopen(filename, "wb");
    if (!fp)
    {
        return false;
    }

    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png_ptr)
    {
        fclose(fp);
        return false;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
    {
        fclose(fp);
        png_destroy_write_struct(&png_ptr, nullptr);
        return false;
    }

    if (setjmp(png_jmpbuf(png_ptr)))
    {
        fclose(fp);
        png_destroy_write_struct(&png_ptr, &info_ptr);
        return false;
    }

    png_init_io(png_ptr, fp);

    int width = qrcode->width;
    int margin = 4;    // Equivalent to border=4 in Python code
    int box_size = 10; // Equivalent to box_size=10 in Python code
    int real_width = (width + margin * 2) * box_size;

    png_set_IHDR(png_ptr, info_ptr, real_width, real_width,
                 8, PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    png_write_info(png_ptr, info_ptr);

    std::vector<unsigned char> row_pixels(real_width, 0xFF); // Initialize to white

    for (int y = -margin; y < width + margin; ++y)
    {
        for (int i = 0; i < box_size; ++i)
        {
            int offset = 0;
            for (int x = -margin; x < width + margin; ++x)
            {
                unsigned char color = 0xFF; // White
                if (x >= 0 && x < width && y >= 0 && y < width)
                {
                    if (qrcode->data[y * width + x] & 0x1)
                    {
                        color = 0x00; // Black
                    }
                }
                for (int j = 0; j < box_size; ++j)
                {
                    row_pixels[offset++] = color;
                }
            }
            png_write_row(png_ptr, row_pixels.data());
        }
    }

    png_write_end(png_ptr, nullptr);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(fp);
    return true;
}

size_t QRCodeGenerator::writeCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t totalSize = size * nmemb;
    std::string *response = static_cast<std::string *>(userp);
    response->append(static_cast<char *>(contents), totalSize);
    return totalSize;
}

void QRCodeGenerator::openQrImage(const std::string &filename)
{
    std::string command = "feh " + filename;
    system(command.c_str());
}

void QRCodeGenerator::generateAndSaveQRCode()
{
    std::string param = sample_id;

    // 生成二维码
    QRcode *qrcode = QRcode_encodeString(param.c_str(), 1, QR_ECLEVEL_L, QR_MODE_8, 1);
    if (!qrcode)
    {
        std::cerr << "Failed to generate QR code." << std::endl;
    }

    if (!saveQrPng("/home/orangepi/Desktop/test/smooth_collect/QRcode.png", qrcode))
    {
        std::cerr << "Failed to save QR code image." << std::endl;
        QRcode_free(qrcode);
    }

    QRcode_free(qrcode);
}

void QRCodeGenerator::Pre_Generate_QRCode()
{
    // std::string user_uuid = "";
    std::string current_time = getCurrentTime();
    std::string device_serial = getDeviceSerial();

    cout << "Getting time" << endl;

    // Generate sample_id
    std::string numbers_and_letters = combineNumLetters(3, 3);
    sample_id = device_serial + "-" + current_time + "-" + numbers_and_letters;
}

std::string QRCodeGenerator::SendUserMessage()
{

    // Prepare parameters
    std::string user_uuid = "";
    std::string param = sample_id;
    cout << "Sample id is:" << sample_id << endl;
    std::string param_encoded;
    CURL *curl = curl_easy_init();
    if (curl)
    {
        char *encoded_sample_id = curl_easy_escape(curl, sample_id.c_str(), sample_id.length());
        if (encoded_sample_id)
        {
            param_encoded = encoded_sample_id;
            curl_free(encoded_sample_id);
        }
        curl_easy_cleanup(curl);
    }

    // Send HTTP GET request
    std::string response_data;
    curl = curl_easy_init();
    if (curl)
    {
        std::string server_url = "http://sp.grifcc.top:8080/collect/get_user";
        std::string full_url = server_url + "?sample_id=" + param_encoded;

        curl_easy_setopt(curl, CURLOPT_URL, full_url.c_str());
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);

        CURLcode res = curl_easy_perform(curl);
        long response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        if (response_code == 200)
        {
            // Parse JSON response
            try
            {
                auto json_response = nlohmann::json::parse(response_data);
                if (json_response.contains("user_uuid"))
                {
                    user_uuid = json_response["user_uuid"].get<std::string>();
                    std::cout << "Success! Retrieved user_uuid: " << user_uuid << std::endl;
                }
                else
                {
                    std::cout << "Failed to retrieve user_uuid from response." << std::endl;
                }
            }
            catch (nlohmann::json::parse_error &e)
            {
                std::cout << "Failed to parse JSON response: " << e.what() << std::endl;
            }
        }
        else
        {
            std::cout << "Request failed with status code " << response_code << ", Response: " << response_data << std::endl;
        }

        curl_easy_cleanup(curl);
    }
    else
    {
        std::cerr << "Failed to initialize CURL." << std::endl;
    }

    std::cout << "Generated sample_id: " << sample_id << std::endl;
    if (!user_uuid.empty() && user_uuid != " ")
    {
        std::cout << "Retrieved user_uuid: " << user_uuid << std::endl;
    }
    user_message = sample_id + "," + user_uuid;

    return user_uuid;
}
