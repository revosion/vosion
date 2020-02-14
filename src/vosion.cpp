#include <vector>
#include <thread>
#include <numeric>
#include <iostream>
#include <chrono>
#include <fstream>
#include <streambuf>
#include <string>
#include <signal.h>
#include <json11.hpp>
#include "vfd.h"
#include "adc.h"
#include "pid.h"
#include "shared_queue.h"
#include <cstdarg>

using namespace json11;
using namespace vosion;
using namespace std;

static sig_atomic_t sigval;
static void onsig(int val)
{
    sigval = (sig_atomic_t)val;
    cout << "Interrupt signal (" << val << ") received.\n";
}

std::string format(const char *format, ...)
{
    va_list args;
    va_start(args, format);
#ifndef _MSC_VER
    size_t size = std::snprintf(nullptr, 0, format, args) + 1; // Extra space for '\0'
    std::unique_ptr<char[]> buf(new char[size]);
    std::vsnprintf(buf.get(), size, format, args);
    return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
#else
    int size = _vscprintf(format, args);
    std::string result(++size, 0);
    vsnprintf_s((char *)result.data(), size, _TRUNCATE, format, args);
    return result;
#endif
    va_end(args);
}

int main()
{
    cout << "Version is 1.001"  << endl;
    
    // Read config file
    ifstream t("config.json");
    string str((istreambuf_iterator<char>(t)), istreambuf_iterator<char>());
    string err;
    const auto config = Json::parse(str, err);

    shared_queue<string> queue;
    queue.push("test");

    // VFD Device setup
    map<string, VarFreqDrive> vfds;

    VarFreqDrive *vfd;

    for (auto &item : config["vfds"]["devices"].array_items())
    {
        vfd = new VarFreqDrive(item, config["vfds"]["commands"]);
        // vfd.start();
        // vfd.stop();
        // vfd.set_frequency(5000);
        const string name = item["name"].string_value();
        // vfds[name] = vfd;
        break;
    }
    vfd->start();  // only pump1 enabled for testing


    // ADC setup
    const int sample_number = config["analog_sensors"]["sample_number"].int_value();
    const int sample_interval = config["analog_sensors"]["sample_interal"].int_value();
    const auto read_interval = chrono::milliseconds(config["analog_sensors"]["read_interval"].int_value());
    vector<ADC> analog_sensors;
    ADC adc0;
    ADC adc1;
    adc1.set_device_num(0);
    adc1.set_device_num(1);
    analog_sensors.push_back(adc0);
    analog_sensors.push_back(adc1);
   
    int sensor_dev_num = config["analog_sensors"]["pressure"]["device_num"].int_value();
    int sensor_chan_num = config["analog_sensors"]["pressure"]["channel_num"].int_value();
    float values[8];
    analog_sensors.at(sensor_dev_num).init();

    // PID controller setup
    const double dt = config["vfds"]["pid_params"]["dt"].number_value();
    const double max = config["vfds"]["pid_params"]["max"].number_value();
    const double min = config["vfds"]["pid_params"]["min"].number_value();
    const double kp = config["vfds"]["pid_params"]["kp"].number_value();
    const double kd = config["vfds"]["pid_params"]["kd"].number_value();
    const double ki = config["vfds"]["pid_params"]["ki"].number_value();
    const double pressureRef = config["vfds"]["pid_params"]["ref"].number_value();   
    PID pid = PID(dt, max, min, kp, kd, ki);

    sigval = 0;
    /* Register signal handlers */
    if (signal(SIGINT, onsig) == SIG_ERR ||
        signal(SIGTERM, onsig) == SIG_ERR ||
        signal(SIGCHLD, SIG_IGN) == SIG_ERR)
    {
        perror("signal");
        return errno;
    }

    // The time at which to reads the next sample, starting now
    ofstream data_file;
    data_file.open("data.csv", ios::out | ios::app);

    auto tm = chrono::steady_clock::now();
    data_file << "start collecting data" << endl;

    while (0 == sigval)
    {
        // Pace the samples to the desired rate
        this_thread::sleep_until(tm);
        analog_sensors.at(sensor_dev_num).read_adc(values);
        
        cout << "all readings for adc " << sensor_dev_num << " is ";
        for (int i = 0; i < 8; i++)
            cout << values[i] << ", ";
        cout << endl;
        cout << "pressure reading is " << values[sensor_chan_num] << endl;
        auto rs = format("%f,%f,%f,%f,%f,%f,%f,%f", values[0],values[1],values[2],values[3],values[4],values[5],values[6],values[7]);
        data_file << rs << endl;

        double pressureOut = values[0];
        double freqRef = pid.calculate(pressureRef, pressureOut);
        cout << "Pump1 Freq is " << freqRef << endl;
        vfd->set_frequency(int(freqRef));

        tm += read_interval;
    }
    cout << "exit..." << endl;
    data_file.close();
    analog_sensors.at(sensor_dev_num).cleanup();
    //analog_sensors.at(1).cleanup();
    vfd->stop();
}
