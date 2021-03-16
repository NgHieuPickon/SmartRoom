#include <SPI.h>
#include "RF24.h"

//const uint64_t pipe = 0xE8E8F0F0E1LL; // адрес канала передачи
const uint64_t pipe = 0x314E6F6465LL; // адрес канала передачи
const char addresses[][6] = {"1Node","2Node"};
RF24 radio(7, 8);
byte msg[7];

void setup() {
  Serial.begin(9600);
  //============================================================Модуль NRF24
  radio.begin();                      // Включение модуля
  radio.setAutoAck(1);                // Установка режима подтверждения приема;
  radio.setDataRate(RF24_250KBPS);    // Устанавливаем скорость
  radio.setChannel(10);               // Устанавливаем канал
  radio.openReadingPipe(1, (const unsigned char *)addresses[0]);     // Открываем 1 канал приема
  radio.startListening();             // Начинаем слушать эфир

  Serial.println("read");

}

void loop() {
  if (radio.available()) {
    while (radio.available()) {
      radio.read(&msg, sizeof(msg));
      Serial.println(msg[0]);
    }
  }
}
