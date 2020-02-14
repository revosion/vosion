#include <thread>
#include <chrono>
#include <fstream>
#include <signal.h>
#include <cstdarg>

#include "cloud_adapter.h"
#include "json.hpp"
#include "sensor.h"
#include "vfd.h"
#include "pid.h"

using namespace nlohmann;
using namespace std;

static bool is_running;

static void onsig(int val) {
  is_running = false;
  cout << is_running << endl;
}

class Record {
 public:
  string name;
  string unit;
  double value;
  Record(string name, string unit, float value)
      :
      name(name),
      unit(unit),
      value(value) {
  }
  ;
  json to_json() const {
    return json { { "n", name }, { "u", unit }, { "v", value }, };
  }
};

int main(int argc, char *argv[]) {
  /* Register signal handlers */
  if (signal(SIGINT, onsig) == SIG_ERR || signal(SIGTERM, onsig) == SIG_ERR
      || signal(SIGCHLD, SIG_IGN) == SIG_ERR) {
    perror ("error registering signal");
    return errno;
  }

  try {
    json config_json;
    std::ifstream config_file("config.json");
    config_file >> config_json;
    cout << "config file read" << config_json.at("cloud_connection") << endl;

    CloudAdapter adapter(config_json.at("cloud_connection"));
    //Sensor sensor(config_json.at("analog_sensors"));
    //VarFreqDrive vfd(config_json.at("vfds"));
    PID pid(config_json.at("vfds").at("pid_params"));

    auto tm = chrono::steady_clock::now();
    float pressure_reading;
    /* Setup code */
    is_running = true;
    while (is_running) {
      tm += chrono::milliseconds(5000);
      this_thread::sleep_until(tm);
      //pressure_reading = sensor.Read("pressure");
      //Record record("test", "V", pressure_reading);
      //adapter.Send(record.to_json());
    }
    //vfd.Cleanup();
    adapter.Cleanup();
  } catch (exception &e) {
    cerr << "Unable to parse configuration file: " << e.what() << endl;
  }
}
