#include <Arduino.h>
#include <OneWire.h>

float ds18b20_getTemp(OneWire &ow);

uint8_t ds18b20_crc8(const uint8_t *data, uint8_t len);

float ds18b20_getTemp(OneWire &ow) {
  uint8_t scratchpad[9];

  ow.reset(); //reset obrigatório
  ow.write(0xCC); // Skip ROM
  ow.write(0x44); // Conversão de temperatura

  delay(750); // Espera conversão 12 bits

  // Leitura scratchpad
  if (!ow.reset()) return NAN;
  ow.write(0xCC); //Skip ROM
  ow.write(0xBE); //Comando de leitura do scratchpad

  for (uint8_t i = 0; i < 9; i++) {
    scratchpad[i] = ow.read();
  }

  if (ds18b20_crc8(scratchpad, 8) != scratchpad[8]) return NAN; // Validação de CRC

  // Converte para temperatura
  int16_t raw = (scratchpad[1] << 8) | scratchpad[0];
  uint8_t cfg = scratchpad[4] & 0x60;

  if (cfg == 0x00) raw &= ~7;      // 9 bits
  else if (cfg == 0x20) raw &= ~3; // 10 bits
  else if (cfg == 0x40) raw &= ~1; // 11 bits

  return (float)raw * 0.0625;
}

uint8_t ds18b20_crc8(const uint8_t *data, uint8_t len) {
  uint8_t crc = 0;
  for (uint8_t i = 0; i < len; i++) {
    uint8_t inbyte = data[i];
    for (uint8_t j = 0; j < 8; j++) {
      uint8_t mix = (crc ^ inbyte) & 0x01;
      crc >>= 1;
      if (mix) crc ^= 0x8C;
      inbyte >>= 1;
    }
  }
  return crc;
}

