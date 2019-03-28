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
#include "mqtt/async_client.h"
#include <json11.hpp>

using namespace std;
using namespace json11;

const int QOS = 1;
const auto PERIOD = chrono::seconds(5);

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

int main(int argc, char *argv[])
{
	std::ifstream t("config.json");
	std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
	std::string err;
	const auto config = Json::parse(str, err);

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
	sslopts.set_trust_store("/vosion/build/amd64/ca.crt");
	connOpts.set_ssl(sslopts);
	const string topic = "channels/" + channel_id + "/messages";

	mqtt::topic top(cli, topic, QOS, true);

	try
	{
		// Connect to the MQTT broker
		cout << "Connecting to server '" << address << "'..." << flush;
		cli.connect(connOpts)->wait();
		cout << "OK\n" << endl;

		// The time at which to reads the next sample, starting now
		auto tm = chrono::steady_clock::now();
		int x = 0;

		while (true)
		{
			// Pace the samples to the desired rate
			this_thread::sleep_until(tm);

			unsigned long ms =
					chrono::duration_cast<std::chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
			Json::object base_json = Json::object{
					{"bn", "pump1"},
					{"bt", double(ms)},
					{"bu", "A"},
					{"bver", 1},
					{"n", "voltage"},
					{"u", "V"},
			};

			float value1 = sin(M_PI / 10 * x / 4) * 120;
			float value2 = sin(M_PI / 15 * x / 4) * 8;
			base_json["v"] = value1;
			Record record("current", "A", value2);
			Json record_json = Json::array{base_json, record};

			std::string pack_json = Json(record_json).dump();
			std::cout << pack_json << std::endl;

			// Publish to the topic
			top.publish(std::move(pack_json));
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
