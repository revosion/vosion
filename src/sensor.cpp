#include <iostream>
#include <exception>
#include <chrono>
#include <iomanip>
#include <chrono>
#include <thread>
#include <vector>

#include "sensor.h"

using namespace nlohmann;
using namespace std;

Sensor::Sensor(json config_json) {
  config_ = config_json;
  // ADC setup
  const int sample_number = config_.at("sample_number");
  const int sample_interval = config_.at("sample_interal");
  const auto read_interval = chrono::milliseconds(config_.at("read_interval"));
  analog_sensors_.at(0).set_device_num(0);
  analog_sensors_.at(1).set_device_num(1);
  analog_sensors_.at(0).init();
  analog_sensors_.at(1).init();
}

Sensor::~Sensor(void) {
  //analog_sensors_.at(0).cleanup();
  //analog_sensors_.at(1).cleanup();
}

float Sensor::Read(string tag) {
  int dev_num = config_.at(tag).at("device_num");
  int chan_num = config_.at(tag).at("channel_num");
  float values[8];
  analog_sensors_.at(dev_num).read_adc(values);
  cout << "all readings for adc " << dev_num << " is ";
  for (int i = 0; i < 8; i++)
    cout << values[i] << ", ";
  cout << endl;
  cout << "pressure reading is " << values[chan_num] << endl;
  return values[chan_num];
}
