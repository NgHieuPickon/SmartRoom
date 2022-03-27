#include <SPI.h>
#include "RF24.h"
const char addresses[][6] = {"1Node", "2Node"};
const uint64_t pipe = 0xE8E8F0F0E1LL; // địa chỉ để phát
RF24 radio(7, 8); //thay 10 thành 53 với mega
byte msg[2];
byte msgRecv[8];
const int sensor = A0;
int value = 0;
unsigned long time = 0;
unsigned long timeDelay = 5000;

void setup() {
  Serial.begin(9600);
  //============================================================Module NRF24
  radio.begin();
  radio.setAutoAck(1);
  radio.setRetries(1, 1);
  radio.setDataRate(RF24_250KBPS);    // Tốc độ truyền
  radio.setPALevel(RF24_PA_MAX);      // Dung lượng tối đa
  radio.setChannel(10);               // Đặt kênh
  radio.openWritingPipe((const unsigned char *)addresses[1]);        // mở kênh
  radio.openReadingPipe(1, (const unsigned char *)addresses[0]);     // Открываем 1 канал приема
  pinMode(sensor, INPUT);
  Serial.println("Start");
}

void loop() {
  msg[0] = 4;
  radio.stopListening();
  delay(50);
  radio.write(&msg, sizeof(msg));
  time = millis();
  delay(50);
  radio.startListening();
  while ((millis() - time) < timeDelay) {
    if (radio.available()) {
      while (radio.available()) {
        radio.read(&msgRecv, sizeof(msgRecv));
        for (int i = 0; i < sizeof(msgRecv); i++) {
          Serial.println(msgRecv[i]);
        }
        Serial.println();
      }
    }
  }
  //  delay(5000);
}
