#include <EEPROM.h>
#include "ESP8266.h"
//#include <softwareserial.h>
//SoftwareSerial mySerial(6, 7);
//如果要用UNO 第3第4要打開，ESP8266.h要把#define ESP8266_USE_SOFTWARE_SERIAL打開!!!!!
const byte max_timeout = 10;
const byte ssid_len = 32;
const byte max_ap = 20;
//char ap_list[max_ap][ssid_len];
char ssidmenu[max_ap][ssid_len];
char WifiSelect ;
char password[]="1004soc1004";

ESP8266 wifi(Serial1);//for MEGA
//for uno
//ESP8266 wifi(mySerial);


void check_wifi(ESP8266 wifi) {
  byte cnt = 0; 
  bool stat = false;
  Serial.println("Checking wifi status.");
  while (!stat && cnt<=max_timeout) {
    stat = wifi.kick();
    ++cnt; Serial.print(".");
    delay(100); yield();
  }
  if (stat)
    Serial.println("\nwifi is up.");
  else {
    Serial.println("\nwifi is down.");
    while (1) yield();
  }
}

void set_wifi_opr(ESP8266 wifi) {
  byte cnt = 0; 
  bool stat = false;
  Serial.println("set wifi to station mode.");
  while (!stat && cnt<=max_timeout) {
    stat = wifi.setOprToStation();
    ++cnt; Serial.print(".");
    delay(100); yield();
  }
  if (stat)
    Serial.println("\nDone.");
  else {
    Serial.println("\nfailed.");
    while (1) yield();
  }
}

void load_ssid(char* src) {
  for (int i=0; i<ssid_len; ++i) {
    src[i] = EEPROM.read(i);
  }
}

void save_ssid(char* ssid) {
  for (int i=0; i<ssid_len; ++i) {
    EEPROM.write(i, ssid[i]);
  }
}

void get_wifi_list(char ap_arr[max_ap][ssid_len], char* raw_str) {
  for (int i=0; i<max_ap; ++i)
    strcpy(ap_arr[i], "");
  byte idx = 0;
  char *ptr = NULL;
  int open_mode = -1;
  char ssid[32];
  ptr = strtok(raw_str, "\n");
  while (ptr != NULL) {
    //Serial.println(ptr);
    sscanf(ptr, "%*[^(](%[^,],\"%[^\"]\"%*[]", &open_mode, ssid);
    
    //if (open_mode == 48)
      strcpy(ap_arr[idx++], ssid);
      
    ptr = strtok(NULL, "\n");
  }
}

void join_wifi(ESP8266 wifi) {
  bool stat = false;
  while (!stat) {
    int cnt = 0;
    char ssid[ssid_len];
    load_ssid(ssid);
    Serial.print("Join AP : ");Serial.println(ssid);
    while (cnt <= 10) {
      ++cnt; Serial.print(".");
      delay(100); yield();
      stat = wifi.joinAP(String(ssid), "");
      if (stat) break;
    }
    if (stat) {
      Serial.println("\nJoin AP success.");
      Serial.println(wifi.getLocalIP().c_str());
    } else {
      Serial.println("\nJoin AP failed.");
      while (1) yield();
    }
  }
}

void setup(void) {
    //mySerial.begin(38400);
    Serial.begin(9600);
    Serial.println("Setup begin");
    
    check_wifi(wifi);
    set_wifi_opr(wifi);
    //join_wifi(wifi);
    char ap_list[max_ap][ssid_len];
    String buf = wifi.getAPList();
    
    get_wifi_list(ap_list, buf.c_str());
    for (int i=0; i<10; ++i) {
      if ((ap_list[i] != NULL) && (ap_list[i][0] == '\0')) {
        // printf("c is empty\n");
        break;
      }
      Serial.print(String(i)+String(" : "));
      Serial.println(ap_list[i]);
    }
    for (int i=0;i<max_ap;i++){
      for(int j=0;j<ssid_len;j++){
      ssidmenu[i][j]=ap_list[i][j];
      //Serial.println(ssidmenu[i]);
      }
    }
    //Serial.print("setup end\r\n");
     Serial.println("\nWhich one do you want to select?");
}

void loop(void) {
  while(Serial.available()>0){
    WifiSelect =Serial.read();
    int selectnumber = (int)WifiSelect - 48;
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssidmenu[selectnumber]);
    
    if(wifi.joinAP(ssidmenu[selectnumber],password)){
        Serial.print("Join AP success\r\n");
        Serial.print("IP: ");       
        Serial.println(wifi.getLocalIP().c_str());
    }else {
        Serial.print("Join AP failure\r\n");
    }
    Serial.print("setup end\r\n");
    //while(1){}
  }
}
