
#include <EEPROM.h>
#include "ESP8266.h"
#include <avr/wdt.h>
const byte max_timeout = 10;
const byte ssid_len = 32;
const byte max_ap = 10;

char ssidmenu[max_ap][ssid_len];
//try pointer array
char *menuTxt[]={};

char WifiSelect ;
char password[]="1004soc1004";

char ap_list[max_ap][ssid_len];
int flagConfirmWifiInitial = 0;
/*---------------------------------------------------------------*/
//LCD
/*
Pinout (header on the top, from left):
  LED   -> 3.3V
  SCK   -> D52
  SDA   -> D51/MOSI
  A0/DC -> D8  or any digital
  RESET -> D9  or any digital
  CS    -> D10 or any digital
  GND   -> GND
  VCC   -> 3.3V
*/
//UNO: https://www.electronicshub.org/wp-content/uploads/2021/01/Arduino-UNO-Pinout.jpg
//MEGA:https://www.arduino.cc/en/reference/SPI 的connection有MOSI MISO SCK SS的對應
#define TFT_CS 10
#define TFT_DC  8
#define TFT_RST 9
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Arduino_ILI9163C_Fast.h>
Arduino_ILI9163C lcd = Arduino_ILI9163C(TFT_DC, TFT_RST, TFT_CS);


#define SCR_WD   128
#define SCR_HT   139.5
#include "RREFont.h"
//#include "rre_kx16x26h.h"
//#include "rre_kx9x14h.h"
#include "rre_fjg_8x16.h"
//#include "rre_vga_8x16.h"
//#include "rre_4x7.h"
#include "rre_5x8.h"
RREFont font;

volatile int encoderPos=0, encoderPosOld=0, encoderStep=2;
#define BUTTON_DOWN 2
#define BUTTON_UP 3
#define BUTTON_Confirm 4
#define BUTTON_Cancel 5

//move up/down
void initEncoder(){
  encoderPos = 0;
  pinMode(BUTTON_Confirm,INPUT_PULLUP);
  pinMode(BUTTON_DOWN,INPUT_PULLUP);
  pinMode(BUTTON_UP,INPUT_PULLUP);
  pinMode( BUTTON_Cancel,INPUT_PULLUP);
}

void customRect(int x, int y, int w, int h, int c) { return lcd.fillRect(x, y, w, h, c); }

const int itemHt = 20;
//const int numMenus = sizeof(ap_list) / sizeof(char*);
int numMenus =16;
const int numScrLines = SCR_HT/itemHt;  // 160/20=8
int menuSel=0, menuSelOld=0;
int menuStart = 0;
int menuMode = -1; // -1 -> menu of options, 0..n -> option
int storedPos = 0;
char buf[100];

uint16_t bgCol     = RGBto565(30,30,140);
uint16_t frameCol  = RGBto565(255,255,40);
uint16_t itemCol   = RGBto565(220,220,220);
uint16_t sliderCol = RGBto565(20,180,180);

void showSelected(char *txt){
  lcd.fillScreen(RGBto565(150,0,150));
  font.setColor(WHITE);  font.printStr(10, 10, "Selected:");
  font.setColor(YELLOW);  font.printStr(10, 30, txt);

}

void printMenuItem(int y, char *item)
{
  font.setColor(itemCol);
  font.printStr(3,2+y*itemHt,item);
}

void printMenu(int full=0)
{
  int n = numMenus<numScrLines ? numMenus : numScrLines;
  for (int i = 0; i < n; i++) {
    formatMenu(ap_list[i + menuStart], buf, 14);
    full ? lcd.fillRect(0,i*itemHt,SCR_WD,itemHt,bgCol) : lcd.fillRect(3,2+i*itemHt,120-4,16,bgCol);
    printMenuItem(i,buf);
  }
}

void formatMenu(char *in, char *out, int num)
{
  int j = strlen(in);
  strncpy(out, in, j);
  for (; j < num; j++) out[j] = ' ';
  out[j] = 0;
}

void drawMenuSlider()
{
  //int ht = 10;
  int ht = (SCR_HT-4)/numMenus;
  int n = (SCR_HT-4-ht) * menuSel / (numMenus - 1);
  lcd.drawRect(SCR_WD-6,0,6,SCR_HT,sliderCol);
  lcd.fillRect(SCR_WD-6+2,2,2,SCR_HT-4,bgCol);
  lcd.fillRect(SCR_WD-6+2,n+2,2,ht,sliderCol);
 
}

void drawFrame(int sel, int stat)
{
  lcd.drawRect(0,(sel-menuStart)*itemHt,120,itemHt-1,stat?frameCol:bgCol);
}

void initMenu()
{
  font.setFont(&rre_fjg_8x16);
  font.setCharMinWd(8);
  font.setSpacing(1);
  font.setColor(WHITE);
  printMenu(1);
  drawMenuSlider();
  drawFrame(menuSel,1);
  
}


int checkButton(int b)
{

  const long btDebounce = 20;
  static long btTime = 0;
  static int lastState = HIGH;
  int val = 0, state = digitalRead(b);
  if (state == LOW && lastState == HIGH) { btTime = millis(); val=0; }
  if (state == HIGH && lastState == LOW && millis()-btTime >= btDebounce) { val=1; }
  lastState = state;
  return val;
}

int checkPressUp(){
  return checkButton(BUTTON_UP);
}
int checkPressDown(){
  return checkButton(BUTTON_DOWN);
}
int checkPressComfirm(){
   
  return checkButton(BUTTON_Confirm);
}
int checkPressCancel(){
   
  return checkButton(BUTTON_Cancel);
}

void setMenu(int m)
{
  menuMode = m;
  storedPos = encoderPos;
  encoderPos = 0;
}

void endMenu(int butt)
{
  if(!butt) return;
  menuMode = -1;
  initMenu();
  encoderPos = storedPos;
}

int holdButton = 0;

void menuItemInit()
{
  
  
      
  setMenu(menuSel);
  //Serial.println(menuMode);
  /*switch(menuMode) {
     
     //case 0: lcd.fillScreen(BLACK); encoderPos=5*encoderStep; break; // for setValue()
     //case 1: showHelp(); break;
     /*case 2: showIntTemp(); break;
     case 3: showBattery(); break;
     case 4: lcd.fillScreen(BLACK); break; // for dumpEEPROM()
     case 6: setColorInit(&bgCol); break;
     case 7: setColorInit(&itemCol); break;
     case 8: setColorInit(&frameCol); break;
     case 9: setColorInit(&sliderCol); break;
     case 11: reqInit(); break;
     default: showSelected(menuTxt[menuSel]);
  }*/
}

void menuItemAction(int butt)
{
  //lcd.fillScreen(RGBto565(150,20,150));
  //font.setColor(WHITE);  font.printStr(10, 10, "SOCLAB");
  showSelected(ap_list[butt]);
  int isPressedDown2 = checkPressDown();
 
  Serial.print("isPressedDown2:");
  Serial.println(isPressedDown2);

  if(isPressedDown2) endMenu(butt);
  /*switch(menuMode) {
     //case 0: setValue(); endMenu(butt); break;
     /*case 4: dumpEEPROM(); endMenu(butt); break;
     case 6: setColorAction(&bgCol,butt); break;
     case 7: setColorAction(&itemCol,butt); break;
     case 8: setColorAction(&frameCol,butt); break;
     case 9: setColorAction(&sliderCol,butt); break;
     case 11: reqAction(); endMenu(butt); break;
  

     //default: endMenu(butt);
    
  }*/
}

void setValue(){}

void handleMenu()
{
  // check press down ?
  // check press up ?
  //int butt = checkButton(); // confirm.
  /*for(int i=0;i<max_ap;i++){
    Serial.println(menuTxt[i]);
  }*/
  
  int isPressedUp = checkPressUp();
  int isPressedDown = checkPressDown();
  int isPressedComfirm = checkPressComfirm();


  if (isPressedUp) encoderPos--;
  if (isPressedDown) encoderPos++;
  if(encoderPos < 0) encoderPos = 0;
  if(encoderPosOld == encoderPos && !isPressedUp && !isPressedComfirm ) return;
  if(encoderPosOld == encoderPos && !isPressedDown && !isPressedComfirm ) return;
  
  /*if(isPressedComfirm==1&& holdButton ==0){
      Serial.println("TEST1");
      //menuMode = 1;
      menuItemInit();
    }
  if(holdButton == 1){
 
       menuMode=-1;
  
    
  }*/

   encoderPosOld = encoderPos;
   if(menuMode == -1){
    menuSel = encoderPos / encoderStep;
    if(menuSel >= numMenus){
      menuSel = numMenus - 1;
      encoderPos = menuSel * encoderStep;
    }
    if(menuSel >= menuStart + numScrLines) {
      menuStart = menuSel - numScrLines + 1;
      printMenu();
    }
    if(menuSel < menuStart) {
      menuStart = menuSel;
      printMenu();
    }
    if(menuSelOld != menuSel) {
      drawFrame(menuSelOld,0);
      drawFrame(menuSel,1);
      drawMenuSlider();
      menuSelOld = menuSel;
    }
    Serial.println(isPressedComfirm);
    if(isPressedComfirm==1) menuItemInit();


   }else if(menuMode!=-1)menuItemAction(menuSel);
  
}







/*---------------------------------------------------------------*/
//WIFI


ESP8266 wifi(Serial1);//for MEGA


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

/*---------------------------------------------------------------*/

int count4=0;
//WIFI_connect
void  Wifi_Connect(){
  //先用SERIAL視窗
  //Serial.println("Setup begin");
  check_wifi(wifi);
  set_wifi_opr(wifi);

  String buf2 = wifi.getAPList();

  get_wifi_list(ap_list, buf2.c_str());
  for (int i=0; i<10; ++i) {
      if ((ap_list[i] != NULL) && (ap_list[i][0] == '\0')) {
        // printf("c is empty\n");
        
        break;
      }
      count4+=1;
  }
  
  numMenus = count4;
    
  


    //Serial.print("setup end\r\n");
     //Serial.println("\nWhich one do you want to select?");

}



void setup() {
  //wdt_disable();
  //WIFI_connect
  Serial.begin(115200);
  Wifi_Connect();
  //Serial.println("wificonnectok");
  



  
   //LCD_initial
  lcd.init();
  font.init(customRect, SCR_WD, SCR_HT); // custom fillRect function and screen width and height values
  initEncoder();
  initMenu();


    
    
}

void loop() {
  
   
  handleMenu();
  
  

}
