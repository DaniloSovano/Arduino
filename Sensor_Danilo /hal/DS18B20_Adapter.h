#ifndef DS18B20_ADAPTER_H
#define DS18B20_ADAPTER_H

#include "GenericTemperatureSensor.h"
#include <DallasTemperature.h>
#include <cstdint>

class DS18B20_Adapter: public GenericTemperatureSensor{
  private:
    OneWire _oneWire;
    DallasTemperature _dallasTemp;
    float _lastTemp ;

  public:
    DS18B20_Adapter(uint8_t pin): _oneWire(pin), _dallasTemp(&_oneWire) {}

    bool begin() override {
      _dallasTemp.begin();
      return true;
    }

    bool read() override {
      _dallasTemp.requestTemperatures();
      float temperature = _dallasTemp.getTempCByIndex(0);

      if(temperature != DEVICE_DISCONNECTED_C){
        _lastTemp = temperature;
        return true;
      }

      _lastTemp = NAN;
      return false;
    }

    float getTemperature() override{
      return _lastTemp;
    }

};


#endif //DS18B20_ADAPTER_H