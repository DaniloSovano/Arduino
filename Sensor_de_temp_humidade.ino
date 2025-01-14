#include "DHT.h"

#define DHTPIN 4
#define DHTTYPE DHT11


DHT dht(DHTPIN,DHTTYPE);


void setup() {

  Serial.begin(9600);
  Serial.print("Inicializando");

  dht.begin();

}

void loop() {
delay(2000);
float temp = dht.readTemperature();

float humidity = dht.readHumidity();

if (isnan(temp) || isnan(humidity)){
  Serial.println("Falha ao obter Resultados");
  return;
}

  Serial.print("Humidade: ");
  Serial.print(humidity);
  Serial.print("%");

  Serial.print("\tTemperatura: ");
  Serial.print(temp);
  Serial.println(" *C");


}


