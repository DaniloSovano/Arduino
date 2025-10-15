#include "HardwareSerial.h"
#ifndef AHT10_ADAPTER_H
#define AHT10_ADAPTER_H

#include "GenericTemperatureSensor.h"
#include <Adafruit_AHTX0.h>

class AHT10_Adapter: public GenericTemperatureSensor{
  private:
    Adafruit_AHTX0 _aht10;
    float _lastTemp = NAN;
    float _lastHum = NAN;

  public:
    AHT10_Adapter() {};

    bool begin() override {
      _aht10.begin();
      return true;
    }

    bool read() override {
      sensors_event_t humidityEvent, tempEvent;

      if(_aht10.getEvent(&humidityEvent, &tempEvent)){
        _lastHum = humidityEvent.relative_humidity;
        _lastTemp = tempEvent.temperature;
        return true;
      }
      _lastTemp = NAN;
      _lastHum = NAN;
      return false;
    }

    float getTemperature() override{
      return _lastTemp;
    }

    float getHumidity() override{
      return _lastHum;
    }
};


#endif //AHT10_ADAPTER_H
