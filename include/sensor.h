#ifndef _SENSOR_H_
#define _SENSOR_H_

#include <vector>

#include "json.hpp"
#include "adc.h"

class Sensor {
 public:
  Sensor(nlohmann::json config_json);
  virtual ~Sensor(void);
  float Read(std::string tag);

 private:
  nlohmann::json config_;
  std::vector<ADC> analog_sensors_{ADC(), ADC()};
};

#endif //_CLOUDADAPTER_H_
