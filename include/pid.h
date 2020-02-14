#ifndef VOSION_PID_H_
#define VOSION_PID_H_

#include "json.hpp"

class PID {
 public:
  // Kp -  proportional gain
  // Ki -  Integral gain
  // Kd -  derivative gain
  // dt -  loop interval time
  // max - maximum value of manipulated variable
  // min - minimum value of manipulated variable
  PID(nlohmann::json config);
  ~PID();

  // Returns the manipulated variable given a setpoint and current process value
  double Calculate(double setpoint, double pv);

 private:
  double dt_;
  double max_;
  double min_;
  double Kp_;
  double Kd_;
  double Ki_;
  double pre_error_;
  double integral_;
  double pressure_ref_;
  bool is_running_ = true;
  void ScheduledThreadFunction(void);
  void Cleanup();
  std::thread scheduledThread_;

};

#endif //VOSION_PID_H_
