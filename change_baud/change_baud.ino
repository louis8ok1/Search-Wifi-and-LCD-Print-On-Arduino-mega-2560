//This code can run AT commands

#include "ESP8266.h"

ESP8266 wifi(Serial1);
void setup() {
  Serial.begin(115200);
  Serial.println("Arduino...OK");
  //if ESP01之前被調成是9600，那begin就要先設9600
  //再來打AT指令:AT+RST
  //接下來關掉SERIAL視窗，把begin改成115200
  //AT指令要看官方手冊
  Serial1.begin(115200);
  Serial1.write("AT+UART_DEF=115200,8,1,0,0\r\n");
  delay(1500);
  Serial1.begin(115200);
  Serial.println("ESP8266...OK");
}

void loop() {
  if (Serial1.available()) {
    
    Serial.write(Serial1.read());
    }
    if (Serial.available()) {
      
      Serial1.write(Serial.read());
      }
}
