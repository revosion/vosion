#include <iostream>
#include <cmath>
#include <chrono>
#include <iostream>
#include <thread>

#include "pid.h"
#include "json.hpp"

using namespace std;
using namespace nlohmann;

PID::PID(json config) {
  dt_ = config.at("dt").get<double>();
  max_ = config.at("max").get<double>();
  min_ = config.at("min").get<double>();
  Kp_ = config.at("kp").get<double>();
  Kd_ = config.at("kd").get<double>();
  Ki_ = config.at("ki").get<double>();
  pressure_ref_ = config.at("ref").get<double>();
  pre_error_ = 0;
  integral_ = 0;
  scheduledThread_ = thread(&PID::ScheduledThreadFunction, this);
}

PID::~PID() {
  Cleanup();
}

double PID::Calculate(double setpoint, double pv) {

  // Calculate error
  double error = setpoint - pv;

  // Proportional term
  double Pout = Kp_ * error;

  // Integral term
  integral_ += error * dt_;
  double Iout = Ki_ * integral_;

  // Restrict to max/min
  if (Iout > max_)
    Iout = max_;
  else if (Iout < min_)
    Iout = min_;

  // Derivative term
  double derivative = (error - pre_error_) / dt_;
  double Dout = Kd_ * derivative;

  // Calculate total output
  double output = Pout + Iout + Dout;

  // Restrict to max/min
  if (output > max_)
    output = max_;
  else if (output < min_)
    output = min_;

  // Save error to previous error
  pre_error_ = error;

  return output;
}

void PID::ScheduledThreadFunction() {
  auto tm = chrono::steady_clock::now();
  while (is_running_) {
    tm += chrono::milliseconds(int(dt_ * 1000));//dt is second in config file
    this_thread::sleep_until(tm);
    double pressure_measured = 1;
    cout << Calculate(pressure_ref_, pressure_measured) << endl;
  }
}

void PID::Cleanup() {
  is_running_ = false;
  scheduledThread_.join();
}

