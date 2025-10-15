
#ifndef GENERICTEMPERATURESENSOR_H
#define GENERICTEMPERATURESENSOR_H
#include <cmath>

class GenericTemperatureSensor{
  public:
  virtual ~GenericTemperatureSensor() {}

  virtual bool begin() = 0;  
  
  virtual bool read() = 0;  
  
  virtual float getTemperature() = 0;  
  
  virtual float getHumidity(){
    return NAN;
  }

};


#endif //GENERICTEMPERATURESENSOR_H
