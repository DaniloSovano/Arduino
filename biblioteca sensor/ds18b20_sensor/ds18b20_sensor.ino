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
  
  
  ds18b20.requestTemperatures();
  Celsius = ds18b20.getTempCByIndex(0);

  Serial.print("Dallas Celsius: ");
  Serial.println(Celsius); 
  delay(3000);

}