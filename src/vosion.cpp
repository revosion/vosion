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

using namespace json11;
using namespace vosion;
using namespace std;

const auto PERIOD = chrono::milliseconds(200);

static sig_atomic_t sigval;
static void onsig(int val)
{
    sigval = (sig_atomic_t)val;
    cout << "Interrupt signal (" << val << ") received.\n";
}

int main()
{
    // Read config file
    ifstream t("config.json");
    string str((istreambuf_iterator<char>(t)), istreambuf_iterator<char>());
    string err;
    const auto config = Json::parse(str, err);

    // VFD Device setup
    map<string, VarFreqDrive> vfds;
    for (auto &item : config["vfds"]["devices"].array_items())
    {
        VarFreqDrive vfd(item, config["vfds"]["commands"]);
        vfd.start();
        vfd.stop();
        vfd.set_frequency(222);
        const string name = item["name"].string_value();
        vfds[name] = vfd;
    }

    // ADC setup
    vector<ADC> adcs;
    ADC adc0;
    ADC adc1;
    adcs.push_back(adc0);
    adcs.push_back(adc1);
    int sensor_dev_num = config["analog_sensors"]["pressure"]["device_num"].int_value();
    int sensor_chan_num = config["analog_sensors"]["pressure"]["channel_num"].int_value();
    float values[8];

    string sensor_dev_num_arg = "-N " + sensor_dev_num;
    std::vector<string> args = {"test", sensor_dev_num_arg, "-g", "-a", "-l 1", "-c 1"};
    // build the pointers array
    std::vector<char *>argv;
    for (std::string &s : args) {
        argv.push_back(&s[0]);
    }
    argv.push_back(NULL);
    adcs.at(sensor_dev_num).init(argv.size() - 1, argv.data());

    // PID controller setup
    const double dt = config["vfds"]["pid_params"]["dt"].number_value();
    const double max = config["vfds"]["pid_params"]["max"].number_value();
    const double min = config["vfds"]["pid_params"]["min"].number_value();
    const double kp = config["vfds"]["pid_params"]["kp"].number_value();
    const double kd = config["vfds"]["pid_params"]["kd"].number_value();
    const double ki = config["vfds"]["pid_params"]["ki"].number_value();
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
    auto tm = chrono::steady_clock::now();
    while (0 == sigval)
    {
        // Pace the samples to the desired rate
        this_thread::sleep_until(tm);
        adcs.at(sensor_dev_num).read_adc(values);

        cout << values[sensor_chan_num] << endl;

        tm += PERIOD;
        //tm += chrono::milliseconds(long(dt*1000));
    }
    cout << "exit..." << endl;
    adcs.at(sensor_dev_num).cleanup();
}