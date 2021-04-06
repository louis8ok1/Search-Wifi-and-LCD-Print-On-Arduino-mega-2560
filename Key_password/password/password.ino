// ILI9163C library example
// (c) 2019 Pawel A. Hernik

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
#define TFT_DC 8
#define TFT_RST 9
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Arduino_ILI9163C_Fast.h>
Arduino_ILI9163C lcd = Arduino_ILI9163C(TFT_DC, TFT_RST, TFT_CS);

#define SCR_WD 128
#define SCR_HT 139.5
#include "RREFont.h"
//#include "rre_kx16x26h.h"
//#include "rre_kx9x14h.h"
#include "rre_fjg_8x16.h"
//#include "rre_vga_8x16.h"
//#include "rre_4x7.h"
#include "rre_5x8.h"
RREFont font;

// -------------------------

volatile int encoderPos = 0, encoderPosOld = 0, encoderStep = 2;
#define BUTTON_DOWN 2
#define BUTTON_UP 3
#define BUTTON_Confirm 4
#define BUTTON_LEFT 6
#define BUTTON_RIGHT 5
//move up/down
void initEncoder()
{
  encoderPos = 0;
  pinMode(BUTTON_Confirm, INPUT_PULLUP);
  pinMode(BUTTON_DOWN, INPUT_PULLUP);
  pinMode(BUTTON_UP, INPUT_PULLUP);
  pinMode(BUTTON_LEFT, INPUT_PULLUP);
  pinMode(BUTTON_RIGHT, INPUT_PULLUP);
  //move down

  //attachInterrupt(digitalPinToInterrupt(BUTTON), pageMoveDown, RISING);
}
/*
void pageMoveDown() {
  encoderPos++;
  Serial.println(encoderPos);
}*/

void customRect(int x, int y, int w, int h, int c) { return lcd.fillRect(x, y, w, h, c); }

char *menuTxt[] = {
    "0 1 2 3 4 5 6",
    "7 8 9 A B C D",
    "E F G H I J K",
    "L M N O P Q R",
    "S T U V W X Y",
    "Z a b c d e f",
    "h i j k l m n",
    "o p q r s t u",
    "v w x y z < >"};

const int itemHt = 20;
const int numMenus = sizeof(menuTxt) / sizeof(char *);
const int numScrLines = SCR_HT / itemHt; // 160/20=8
int menuSel = 0, menuSelOld = 0;
int menuStart = 0;
int menuMode = -1; // -1 -> menu of options, 0..n -> option
int storedPos = 0;
char buf[100];

//*---------------------------------------
const int itemWidth = 15;
const int numMenus_2 = 13;
const int numScirLines_2 = SCR_WD / itemWidth;
int menuSel_2 = 0, menuSelOld_2 = 0;
int menuStart_2 = 0;
int direction = -1;
int storedPos_2 = -1;
//password

char password[32];

int menuMode_2 = -1;

volatile int encoderPos_2 = 0, encoderPosOld_2 = 0, encoderStep_2 = 2; //choose LINE

uint16_t bgCol = RGBto565(30, 30, 140);
uint16_t frameCol = RGBto565(255, 255, 40);
uint16_t itemCol = RGBto565(220, 220, 220);
uint16_t sliderCol = RGBto565(20, 180, 180);

void showSelected(char *txt)
{
  lcd.fillScreen(RGBto565(150, 0, 150));
  font.setColor(WHITE);
  font.printStr(10, 10, "Selected:");
  font.setColor(YELLOW);
  font.printStr(10, 30, txt);
}

void printMenuItem(int y, char *item)
{
  font.setColor(itemCol);
  font.printStr(3, 2 + y * itemHt, item);
}

void printMenu(int full = 0)
{
  int n = numMenus < numScrLines ? numMenus : numScrLines;
  for (int i = 0; i < n; i++)
  {
    formatMenu(menuTxt[i + menuStart], buf, 16);
    full ? lcd.fillRect(0, i * itemHt, SCR_WD, itemHt, bgCol) : lcd.fillRect(3, 2 + i * itemHt, 120 - 4, 16, bgCol);
    printMenuItem(i, buf);
  }
}

void formatMenu(char *in, char *out, int num)
{
  int j = strlen(in);
  strncpy(out, in, j);
  for (; j < num; j++)
    out[j] = ' ';
  out[j] = 0;
}

/*畫最右邊進度條*/
void drawMenuSlider()
{
  //int ht = 10;
  int ht = (SCR_HT - 4) / numMenus;
  int n = (SCR_HT - 4 - ht) * menuSel / (numMenus - 1);
  lcd.drawRect(SCR_WD - 6, 0, 6, SCR_HT, sliderCol);
  lcd.fillRect(SCR_WD - 6 + 2, 2, 2, SCR_HT - 4, bgCol);
  lcd.fillRect(SCR_WD - 6 + 2, n + 2, 2, ht, sliderCol);
}
/*畫框框*/
void drawFrame(int sel, int stat)
{
  lcd.drawRect(0, (sel - menuStart) * itemHt, 15, itemHt - 1, stat ? frameCol : bgCol);
}
//x coordinate , y coordinate, Width in pixels, Height in pixels,color
void drawFrame_2(int butt, int sel, int stat)
{
  lcd.drawRect((sel - menuStart_2) * 18, (butt - menuStart) * itemHt, 15, itemHt - 1, stat ? frameCol : bgCol);
}

void initMenu()
{
  font.setFont(&rre_fjg_8x16);
  font.setCharMinWd(8);
  font.setSpacing(1);
  font.setColor(WHITE);
  printMenu(1);
  drawMenuSlider();
  drawFrame(menuSel, 1);
}

// -----------------------------------------------

// 0=idle, 1,2,3=click,
int checkButton(int b)
{

  const long btDebounce = 20;
  static long btTime = 0;
  static int lastState = HIGH;
  int val = 0, state = digitalRead(b);
  if (state == LOW && lastState == HIGH)
  {
    btTime = millis();
    val = 0;
  }
  if (state == HIGH && lastState == LOW && millis() - btTime >= btDebounce)
  {
    val = 1;
  }
  lastState = state;
  return val;
}

/*int prevButtonState=0;*/

/*int handleButton()
{
  prevButtonState = buttonState;
  buttonState = checkButton();
  return buttonState;
}*/
//-----------------------------------------------

int checkPressUp()
{
  return checkButton(BUTTON_UP);
}
int checkPressDown()
{
  return checkButton(BUTTON_DOWN);
}
int checkPressComfirm()
{

  return checkButton(BUTTON_Confirm);
}
int checkPressLEFT()
{

  return checkButton(BUTTON_LEFT);
}

int checkPressRIGHT()
{

  return checkButton(BUTTON_RIGHT);
}

//-----------------------------------------------
void setMenu(int m)
{
  menuMode = m;
  storedPos = encoderPos;
  encoderPos = 0;
}

void endMenu(int butt)
{
  if (!butt)
    return;
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
  showSelected(menuTxt[butt]);
  int isPressedDown2 = checkPressDown();

  Serial.print("isPressedDown2:");
  Serial.println(isPressedDown2);

  if (isPressedDown2)
    endMenu(butt);
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
void setValue()
{
}
//---------------------------------------------

void password_func(int butt)
{

  int isPressedLeft = checkPressLEFT();
  int isPressedRight = checkPressRIGHT();
  int isPressedComfirm = checkPressComfirm();
  int isPressedUp = checkPressUp();
  int isPressedDown = checkPressDown();

  if (isPressedLeft)
  {
    encoderPos_2 -= 2;
    Serial.print('1');
  }

  if (isPressedRight)
  {
    encoderPos_2 += 2;
    Serial.print('2');
  }
  if (isPressedDown)
  {
    Serial.println("TEST1111111");

    direction = -1;
    lcd.fillScreen(RGBto565(255, 255, 255));
    initMenu();
    menuMode = -1;
    encoderPos_2 =0;
    butt =0;
  }

  if (encoderPos_2 < 0)
    encoderPos_2 = 0;
  if (encoderPosOld_2 == encoderPos_2 && !isPressedLeft && !isPressedComfirm)
    return;
  if (encoderPosOld_2 == encoderPos_2 && !isPressedRight && !isPressedComfirm)
    return;
  encoderPosOld_2 = encoderPos_2;
  //在PC上顯示所選字母
  /*
  if (isPressedComfirm)
  {
    Serial.print("word: ");
    Serial.println(menuTxt[butt][encoderPosOld_2]);
  }*/
  if (menuMode_2 == -1)
  {

    menuSel_2 = encoderPos_2 / encoderStep_2;

    if (menuSel_2 >= menuStart_2 + numScirLines_2)
    {

      menuStart_2 = menuSel_2 - numScirLines_2 + 1;
      Serial.println(menuStart_2);
      printMenu();
    }
    if (menuSel_2 < menuStart_2)
    {
      menuStart_2 = menuSel_2;
      printMenu();
    }
    if (menuSelOld_2 != menuSel_2)
    {
      //TODO: 要改drawFrame，每按一次要往左右邊跑，所以要改變它的X,y
      drawFrame_2(butt, menuSelOld_2, 0);
      drawFrame_2(butt, menuSel_2, 1);
      //drawMenuSlider();
      menuSelOld_2 = menuSel_2;
    } /*
    if (isPressedComfirm)
    { 
      Serial.print("encoderPosOld_2: ");
      Serial.println(encoderPosOld_2);
      Serial.print("word: ");
      Serial.println(menuTxt[butt][encoderPosOld_2]);
    }*/
    //TODO:在LCD上顯示所選的字
    if (isPressedComfirm)
    {
      //Serial.println(menuTxt[butt][encoderPosOld_2]);
      strncat(password, &menuTxt[butt][encoderPosOld_2], 1);
      Serial.print("Select: ");
      Serial.println(password);
    }
  }
  Serial.println(isPressedDown);
}

void handleMenu()
{

  // check press down ?
  // check press up ?
  //int butt = checkButton(); // confirm.
  int isPressedUp = checkPressUp();
  int isPressedDown = checkPressDown();
  int isPressedComfirm = checkPressComfirm();
  //TODO: 這邊+=2也可以解決連按

  if (isPressedUp)
  {
    encoderPos--;
    Serial.print("Up");
  }

  if (isPressedDown)
    encoderPos++;
  if (encoderPos < 0)
    encoderPos = 0;
  if (encoderPosOld == encoderPos && !isPressedUp && !isPressedComfirm)
    return;
  if (encoderPosOld == encoderPos && !isPressedDown && !isPressedComfirm)
    return;

  /*if(isPressedComfirm==1&& holdButton ==0){
      Serial.println("TEST1");
      //menuMode = 1;
      menuItemInit();
    }
  if(holdButton == 1){
 
       menuMode=-1;
  
    
  }*/
  encoderPosOld = encoderPos;
  if (menuMode == -1)
  {

    menuSel = encoderPos / encoderStep;
    Serial.print("menuSel: ");
    Serial.println(menuSel);
    Serial.print("menuSelOld: ");
    Serial.println(menuSelOld);
    if (menuSel >= numMenus)
    {
      menuSel = numMenus - 1;
      //encoderPos = menuSel * encoderStep;
    }
    if (menuSel >= menuStart + numScrLines)
    {
      menuStart = menuSel - numScrLines + 1;
      printMenu();
    }
    if (menuSel < menuStart)
    {
      menuStart = menuSel;
      printMenu();
    }
    if (menuSelOld != menuSel)
    {
      drawFrame(menuSelOld, 0);
      drawFrame(menuSel, 1);
      drawMenuSlider();
      menuSelOld = menuSel;
    }

    if (isPressedComfirm == 1)
    {

      menuItemInit();
      direction = 1;
      return;
    }
  }
}
/*
void handleMenu()
{
  int butt = readButton();
  if(encoderPos < 0) encoderPos = 0;
  if(encoderPosOld == encoderPos && !butt) return;
  encoderPosOld = encoderPos;
  if(menuMode == -1) {
    menuSel = encoderPos / encoderStep;
    if(menuSel >= numMenus) {
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
    if(butt) menuItemInit();
  } else menuItemAction(butt);
}*/
void setup(void)
{
  Serial.begin(9600);

  lcd.init();
  font.init(customRect, SCR_WD, SCR_HT); // custom fillRect function and screen width and height values
  initEncoder();
  initMenu();
}

void loop()
{
  if (direction == -1)
    handleMenu();
  else if (direction != -1)

    password_func(menuSel);

  //delay(110);
  //有DELAY會連按
}
