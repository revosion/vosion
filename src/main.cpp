#include <random>
#include <string>
#include <thread>
#include <chrono>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <streambuf>
#include <signal.h>
#include "mqtt/async_client.h"
#include <json11.hpp>
#include "adc.h"
#include "sender.h"
#include "receiver.h"

using namespace std;
using namespace json11;

const int QOS = 1;
const auto PERIOD = chrono::milliseconds(5000);

class Record
{
public:
    string name;
    string unit;
    float value;
    Record(string name, string unit, float value) : name(name), unit(unit), value(value){};
    Json to_json() const
    {
        return Json::object{
            {"n", name},
            {"u", unit},
            {"v", value},
        };
    }
};

static sig_atomic_t sigval;

static void onsig(int val)
{
    sigval = (sig_atomic_t)val;
}

int main(int argc, char *argv[])
{
    std::ifstream t("config.json");
    std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
    std::string err;
    const auto config = Json::parse(str, err);

    // MQTT setup
    const string address = config["cloud_connection"]["cloud_url"].string_value();
    const string channel_id = config["cloud_connection"]["channel_id"].string_value();
    const string thing_id = config["cloud_connection"]["thing_id"].string_value();
    const string thing_key = config["cloud_connection"]["thing_key"].string_value();
    const int max_buffered_msgs = config["cloud_connection"]["max_buffered_msgs"].int_value();
    const string persist_dir = config["cloud_connection"]["persist_dir"].string_value();

    mqtt::async_client cli(address, "", max_buffered_msgs, persist_dir);
    mqtt::connect_options connOpts;
    connOpts.set_keep_alive_interval(max_buffered_msgs * PERIOD);
    connOpts.set_clean_session(true);
    connOpts.set_automatic_reconnect(true);
    connOpts.set_user_name(thing_id);
    connOpts.set_password(thing_key);
    mqtt::ssl_options sslopts;
    sslopts.set_trust_store("ca.crt");
    connOpts.set_ssl(sslopts);
    const string topic = "channels/" + channel_id + "/messages";

    mqtt::topic top(cli, topic, QOS, true);

    // CAN Device setup
    map<string, Sender> vfds;
    for (auto &item : config["vfds"].array_items())
    {
        char *can_iface = (char *)item["can_iface"].string_value().c_str();
        const string name = item["name"].string_value();
        Sender sender(can_iface);
        vfds[name] = sender;
    }

    Sender pump_4 = vfds.find("pump_4")->second;
    int msgid = 819;
    Json::object start_msg;
    start_msg["param_addr"] = config["commands"]["start"]["param_addr"];
    start_msg["param_val"] = config["commands"]["start"]["param_val"];
    start_msg["operation"] = config["commands"]["start"]["operation"];
    start_msg["slave_id"] = 4;

    Json::object stop_msg;
    stop_msg["param_addr"] = config["commands"]["stop"]["param_addr"];
    stop_msg["param_val"] = config["commands"]["stop"]["param_val"];
    stop_msg["operation"] = config["commands"]["stop"]["operation"];
    stop_msg["slave_id"] = 4;

    Json::object set_frequency_msg;
    set_frequency_msg["param_addr"] = config["commands"]["set_frequency"]["param_addr"];
    set_frequency_msg["operation"] = config["commands"]["set_frequency"]["operation"];
    set_frequency_msg["slave_id"] = 4;

    /* 	pump_4.Send(msgid, start_msg);
  auto tm = chrono::steady_clock::now();
  int x = 0;
  while (true) {
    this_thread::sleep_until(tm);
    int frequency = (int)(sin(M_PI / 10 * x / 4) * 500) + 4500;
    cout << frequency << endl;
    set_frequency_msg["param_val"] = frequency;
    pump_4.Send(msgid, set_frequency_msg);

    tm += PERIOD;
    x++;
  } */

    //ADC sensors setup
    ADC adc0(0);
    ADC adc1(1);
    float values[2][8];
    int sensor_dev_num;
    int sensor_chan_num;
    for (auto &item : config["analog_sensors"].array_items())
    {
        if (item["name"].string_value() == "pressure_1")
        {
            sensor_dev_num = item["adc_dev"].int_value();
            sensor_chan_num = item["channel"].int_value();
        }
    }

    try
    {
        // Connect to the MQTT broker
        cout << "Connecting to server '" << address << "'..." << flush;
        //cli.connect(connOpts)->wait();
        cout << "OK\n"
             << endl;

        // The time at which to reads the next sample, starting now
        auto tm = chrono::steady_clock::now();
        int x = 0;

        //sender.Close();

        sigval = 0;
        /* Register signal handlers */
        if (signal(SIGINT, onsig) == SIG_ERR ||
            signal(SIGTERM, onsig) == SIG_ERR ||
            signal(SIGCHLD, SIG_IGN) == SIG_ERR)
        {
            perror("signal");
            return errno;
        }

        while (0 == sigval)
        {
            // Pace the samples to the desired rate
            this_thread::sleep_until(tm);

            adc0.read_adc(values[0]);
            adc1.read_adc(values[1]);

            auto ms =
                chrono::duration_cast<std::chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
            cout << ms << endl;
            Json::object base_json = Json::object{
                {"bn", "pump1"},
                {"bt", double(ms)},
                {"bu", "A"},
                {"bver", 1},
                {"n", "sim"},
                {"u", "V"},
            };

            float sim_value = sin(M_PI / 10 * x / 4) + 1;
            base_json["v"] = sim_value;
            vector<Json> record_json;
            record_json.push_back(base_json);
            for (int i = 0; i < 8; i++)
            {
                string name = "adc_vin";
                name += std::to_string(i);
                Record record(name, "V", values[0][i]);
                record_json.push_back(record);
            }

            std::string pack_json = Json(record_json).dump();
            std::cout << pack_json << std::endl;

            // Publish to the topic
            //top.publish(std::move(pack_json));
            x++;
            tm += PERIOD;
        }

        // Disconnect
        cout << "\nDisconnecting..." << flush;
        cli.disconnect()->wait();
        cout << "OK" << endl;
    }
    catch (const mqtt::exception &exc)
    {
        cerr << exc.what() << endl;
        return 1;
    }

    return 0;
}
