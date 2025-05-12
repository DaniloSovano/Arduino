#include "esp32-hal-gpio.h"


static uint8_t pin

//Função de inicialização
void ds18b20_init(pin){

    ds_pin = pin;
    pinMode(ds_pin, OUTPUT);
    digitalWrite(ds_pin, HIGH);
}
static uint8_t ds_pin; // guardaremos o pino aqui

void ds18b20_init(uint8_t pin) {
  ds_pin = pin;
  pinMode(ds_pin, OUTPUT);
  digitalWrite(ds_pin, HIGH);
}

// Função de reset do barramento 1-Wire 
static bool onewire_reset() {
  pinMode(ds_pin, OUTPUT);
  digitalWrite(ds_pin, LOW);
  delayMicroseconds(480);

  pinMode(ds_pin, INPUT); 
  delayMicroseconds(70);

  bool presence = (digitalRead(ds_pin) == LOW); 

  delayMicroseconds(410);

  return presence;
}
