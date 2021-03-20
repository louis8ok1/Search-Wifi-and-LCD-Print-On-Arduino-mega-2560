
//-----------------------------------------
WIFI
#include <EEPROM.h>
#include "ESP8266.h"

const byte max_timeout = 10;
const byte ssid_len = 32;
const byte max_ap = 20;

char ssidmenu[max_ap][ssid_len];
char WifiSelect ;
char password[]="1004soc1004";

ESP8266 wifi(Serial1);//for MEGA






void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:

}
