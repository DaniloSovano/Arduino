#include <OneWire.h>
#include <DallasTemperature.h>
#include "htmlPage.h"

OneWire oneWire(4);
DallasTemperature ds18b20(&oneWire);
float Celsius;
void setup() {
  Serial.begin(9600);

  ds18b20.begin();
}
void loop() {
  
  float temp = ds18b20_getTemp(oneWire);
  ds18b20.requestTemperatures();
  Celsius = ds18b20.getTempCByIndex(0);
  
  Serial.print("Dallas Celsius: ");
  Serial.println(Celsius); 

  Serial.print("Ds18b20 Celsius: ");
  Serial.println(temp); 

  delay(3000);

}