#include <Arduino.h>


//Serial Rx using EasyTransfer lib
#include <EasyTransfer.h>

/***************************************************
  This is our GFX example for the Adafruit ILI9341 Breakout and Shield
  ----> http://www.adafruit.com/products/1651

  Check out the links above for our tutorials and wiring diagrams
  These displays use SPI to communicate, 4 or 5 pins are required to
  interface (RST is optional)
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/


#include "SPI.h"

#include <Adafruit_GFX_AS.h>    // Core graphics library, with extra fonts.
#include <Adafruit_ILI9341_STM.h> // STM32 DMA Hardware-specific library

// For the Adafruit shield, these are the default.
#define TFT_CS         PA0
#define TFT_DC         PA2
#define TFT_RST        PA1

#define TFT_LED_PWM    PB7


// Use hardware SPI (on Uno, #13, #12, #11) and the above for CS/DC
//Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);seo
// If using the breakout, change pins as desired
//Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);
Adafruit_ILI9341_STM tft = Adafruit_ILI9341_STM(TFT_CS, TFT_DC, TFT_RST); // Use hardware SPI

//Serial Rx setup
//create object
EasyTransfer ET;

struct RECEIVE_DATA_STRUCTURE{
  //put your variable definitions here for the data you want to receive
  //THIS MUST BE EXACTLY THE SAME ON THE OTHER ARDUINO
  uint8_t boost;
  uint8_t throttle;
  uint8_t injector;
  uint8_t timing;
};

//give a name to the group of data
RECEIVE_DATA_STRUCTURE mydata;
RECEIVE_DATA_STRUCTURE curr_data;

#define BST_BARX  2
#define THR_BARX  82
#define TIM_BARX  152

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  Serial.begin(115200);
  Serial.println("STM32 LCD Backpack ILI9341");
  tft.begin();

  // read diagnostics (optional but can help debug problems)
  uint8_t x = tft.readcommand8(ILI9341_RDMODE);
  Serial.print("Display Power Mode: 0x"); Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDMADCTL);
  Serial.print("MADCTL Mode: 0x"); Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDPIXFMT);
  Serial.print("Pixel Format: 0x"); Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDIMGFMT);
  Serial.print("Image Format: 0x"); Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDSELFDIAG);
  Serial.print("Self Diagnostic: 0x"); Serial.println(x, HEX);

  // Setup TFT backlight control
  pinMode(TFT_LED_PWM, OUTPUT);
  analogWrite(TFT_LED_PWM, 128);

  screen1();

  //start the library, pass in the data details and the name of the serial port. Can be Serial, Serial1, Serial2, etc.
  ET.begin(details(mydata), &Serial);

  digitalWrite(LED_BUILTIN, HIGH);
}

// serial debug circle
uint8_t status;
uint8_t status_serial;

// main loop
void loop(void) {
  //check and see if a data packet has come in.
  if(ET.receiveData()){
    //this is how you access the variables. [name of the group].[variable name]
    curr_data = mydata;
    uint8_t val_bst = curr_data.boost;
    graph_boost(val_bst);
    //graph_throttle(curr_data.throttle);
    graph_timing(curr_data.timing);

    // serial debug circle
    status ? status = 0 : status = 1;
    tft.fillCircle(315, 5, 2, status ? ILI9341_WHITE:ILI9341_BLACK);
  }

  if(Serial.available()){
    // serial debug
    status_serial ? status_serial = 0 : status_serial = 1;
    status_serial ? digitalWrite(LED_BUILTIN, LOW):digitalWrite(LED_BUILTIN, HIGH);
  }




  // while(1){
  //   for(int i =0 ; i< 256; i++){
  //     graph(i);
  //     //Serial.print(i);
  //     //Serial.print(", ");
  //     delay(50);
  //   }
  // }

}

void screen1(){
  tft.fillScreen(ILI9341_BLACK);
  tft.setRotation(3);

  tft.setCursor(2, BST_BARX);
  tft.setTextColor(ILI9341_WHITE);  tft.setTextSize(3);
  tft.println("Bst:");
  tft.drawRect(0, BST_BARX+28, tft.width(), 50, ILI9341_WHITE);

  tft.setCursor(2, THR_BARX);
  tft.setTextColor(ILI9341_WHITE);  tft.setTextSize(2);
  tft.println("Thr:");
  tft.drawRect(0, THR_BARX+18, tft.width(), 50, ILI9341_WHITE);

  tft.setCursor(2, TIM_BARX);
  tft.setTextColor(ILI9341_WHITE);  tft.setTextSize(2);
  tft.println("Tim:");
  tft.drawRect(0, TIM_BARX+18, tft.width(), 50, ILI9341_WHITE);

}

uint8_t peak_MRP;
uint32_t oldtime;
void graph_boost(uint8_t val){
  // map the value into the size needed
  long data = map(val, 0, 255, 0, tft.width()-2); // bar graph
  float display_data = (map(val, 0, 255, -100, 155)); // convert to MRP Bar
  display_data = display_data /100;
  if (val > peak_MRP){ // check peak
    peak_MRP = val;
    oldtime = millis();
  }else if(millis() - oldtime > 2500){
    peak_MRP = 0;
  }
  long pk_data = map(peak_MRP, 0, 255, 0, tft.width()-2); // bar graph
  float display_pk = (map(peak_MRP, 0, 255, -100, 155)); // convert to MRP Bar
  display_pk = display_pk/100;

  // draw the data
  if(display_data < 0 ){ //blue
    tft.fillRect(1, 31, data, 48, ILI9341_BLUE);
  }else if(display_data >=0 && display_data <0.9){
    tft.fillRect(1, 31, data, 48, ILI9341_GREEN);
  }else if(display_data >=0.9 && display_data <1.1){
    tft.fillRect(1, 31, data, 48, ILI9341_YELLOW);
  }else{
    tft.fillRect(1, 31, data, 48, ILI9341_RED);
  }
  tft.fillRect(data+1 , 31, tft.width()-2-data  , 48, ILI9341_BLACK);
  tft.fillRect(pk_data-1, 31, 2, 48, ILI9341_ORANGE); // peak marker
  //Serial.println(data);

  // print value
  tft.setCursor(70, 2);
  tft.setTextColor(ILI9341_WHITE,ILI9341_BLACK);
  if (display_data >= 0) tft.print(" ");
  tft.print(display_data);

  // print peak
  tft.setCursor(170, 2);
  tft.print("Pk:");
  if (display_pk >= 0) tft.print(" ");
  tft.print(display_pk);

}

void graph_throttle(uint8_t val){
  // map the value into the size needed
  long data = map(val, 0, 100, 0, tft.width()-2); // bar graph
  float display_data = val;


  // draw the data
  if(display_data < 0 ){ //blue
    tft.fillRect(1, THR_BARX+19, data, 48, ILI9341_BLUE);
  }else if(display_data >=0 && display_data <0.9){
    tft.fillRect(1, THR_BARX+19, data, 48, ILI9341_GREEN);
  }else if(display_data >=0.9 && display_data <1.1){
    tft.fillRect(1, THR_BARX+19, data, 48, ILI9341_YELLOW);
  }else{
    tft.fillRect(1, THR_BARX+19, data, 48, ILI9341_RED);
  }
  tft.fillRect(data+1 , THR_BARX+19, tft.width()-2-data  , 48, ILI9341_BLACK);

  // print value
  tft.setCursor(70, THR_BARX);
  tft.setTextColor(ILI9341_WHITE,ILI9341_BLACK);
  if (display_data >= 0) tft.print(" ");
  tft.print(display_data);

}

void graph_timing(uint8_t val){
  // map the value into the size needed
  float fval = val;
  fval = (fval/2)-64 ; // convert to deg btdc
  long data = map(val, 0, 255, 0, tft.width()-2); // bar graph
  float display_data = fval;


  // draw the data
  if(display_data < 0 ){ //blue
    tft.fillRect(1, TIM_BARX+19, data, 48, ILI9341_BLUE);
  }else if(display_data >=0 && display_data <0.9){
    tft.fillRect(1, TIM_BARX+19, data, 48, ILI9341_GREEN);
  }else if(display_data >=0.9 && display_data <1.1){
    tft.fillRect(1, TIM_BARX+19, data, 48, ILI9341_YELLOW);
  }else{
    tft.fillRect(1, TIM_BARX+19, data, 48, ILI9341_RED);
  }
  tft.fillRect(data+1 , TIM_BARX+19, tft.width()-2-data  , 48, ILI9341_BLACK);

  // print value
  tft.setCursor(70, TIM_BARX);
  tft.setTextColor(ILI9341_WHITE,ILI9341_BLACK);
  if (display_data >= 0) tft.print(" ");
  tft.print(display_data);

}

unsigned long testFillScreen() {
  unsigned long start = micros();
  tft.fillScreen(ILI9341_BLACK);
  tft.fillScreen(ILI9341_RED);
  tft.fillScreen(ILI9341_GREEN);
  tft.fillScreen(ILI9341_BLUE);
  tft.fillScreen(ILI9341_BLACK);
  return micros() - start;
}

unsigned long testText() {
  tft.fillScreen(ILI9341_BLACK);
  unsigned long start = micros();
  tft.setCursor(0, 0);
  tft.setTextColor(ILI9341_WHITE);  tft.setTextSize(1);
  tft.println("Hello World!");
  tft.setTextColor(ILI9341_YELLOW); tft.setTextSize(2);
  tft.println(1234.56);
  tft.setTextColor(ILI9341_RED);    tft.setTextSize(3);
  tft.println(0xDEADBEEF, HEX);
  tft.println();
  tft.setTextColor(ILI9341_GREEN);
  tft.setTextSize(5);
  tft.println("Groop");
  tft.setTextSize(2);
  tft.println("I implore thee,");
  tft.setTextSize(1);
  tft.println("my foonting turlingdromes.");
  tft.println("And hooptiously drangle me");
  tft.println("with crinkly bindlewurdles,");
  tft.println("Or I will rend thee");
  tft.println("in the gobberwarts");
  tft.println("with my blurglecruncheon,");
  tft.println("see if I don't!");
  return micros() - start;
}

unsigned long testLines(uint16_t color) {
  unsigned long start, t;
  int           x1, y1, x2, y2,
                w = tft.width(),
                h = tft.height();

  tft.fillScreen(ILI9341_BLACK);

  x1 = y1 = 0;
  y2    = h - 1;
  start = micros();
  for (x2 = 0; x2 < w; x2 += 6) tft.drawLine(x1, y1, x2, y2, color);
  x2    = w - 1;
  for (y2 = 0; y2 < h; y2 += 6) tft.drawLine(x1, y1, x2, y2, color);
  t     = micros() - start; // fillScreen doesn't count against timing

  tft.fillScreen(ILI9341_BLACK);

  x1    = w - 1;
  y1    = 0;
  y2    = h - 1;
  start = micros();
  for (x2 = 0; x2 < w; x2 += 6) tft.drawLine(x1, y1, x2, y2, color);
  x2    = 0;
  for (y2 = 0; y2 < h; y2 += 6) tft.drawLine(x1, y1, x2, y2, color);
  t    += micros() - start;

  tft.fillScreen(ILI9341_BLACK);

  x1    = 0;
  y1    = h - 1;
  y2    = 0;
  start = micros();
  for (x2 = 0; x2 < w; x2 += 6) tft.drawLine(x1, y1, x2, y2, color);
  x2    = w - 1;
  for (y2 = 0; y2 < h; y2 += 6) tft.drawLine(x1, y1, x2, y2, color);
  t    += micros() - start;

  tft.fillScreen(ILI9341_BLACK);

  x1    = w - 1;
  y1    = h - 1;
  y2    = 0;
  start = micros();
  for (x2 = 0; x2 < w; x2 += 6) tft.drawLine(x1, y1, x2, y2, color);
  x2    = 0;
  for (y2 = 0; y2 < h; y2 += 6) tft.drawLine(x1, y1, x2, y2, color);

  return micros() - start;
}

unsigned long testFastLines(uint16_t color1, uint16_t color2) {
  unsigned long start;
  int           x, y, w = tft.width(), h = tft.height();

  tft.fillScreen(ILI9341_BLACK);
  start = micros();
  for (y = 0; y < h; y += 5) tft.drawFastHLine(0, y, w, color1);
  for (x = 0; x < w; x += 5) tft.drawFastVLine(x, 0, h, color2);

  return micros() - start;
}

unsigned long testRects(uint16_t color) {
  unsigned long start;
  int           n, i, i2,
                cx = tft.width()  / 2,
                cy = tft.height() / 2;

  tft.fillScreen(ILI9341_BLACK);
  n     = min(tft.width(), tft.height());
  start = micros();
  for (i = 2; i < n; i += 6) {
    i2 = i / 2;
    tft.drawRect(cx - i2, cy - i2, i, i, color);
  }

  return micros() - start;
}

unsigned long testFilledRects(uint16_t color1, uint16_t color2) {
  unsigned long start, t = 0;
  int           n, i, i2,
                cx = tft.width()  / 2 - 1,
                cy = tft.height() / 2 - 1;

  tft.fillScreen(ILI9341_BLACK);
  n = min(tft.width(), tft.height());
  for (i = n; i > 0; i -= 6) {
    i2    = i / 2;
    start = micros();
    tft.fillRect(cx - i2, cy - i2, i, i, color1);
    t    += micros() - start;
    // Outlines are not included in timing results
    tft.drawRect(cx - i2, cy - i2, i, i, color2);
  }

  return t;
}

unsigned long testFilledCircles(uint8_t radius, uint16_t color) {
  unsigned long start;
  int x, y, w = tft.width(), h = tft.height(), r2 = radius * 2;

  tft.fillScreen(ILI9341_BLACK);
  start = micros();
  for (x = radius; x < w; x += r2) {
    for (y = radius; y < h; y += r2) {
      tft.fillCircle(x, y, radius, color);
    }
  }

  return micros() - start;
}

unsigned long testCircles(uint8_t radius, uint16_t color) {
  unsigned long start;
  int           x, y, r2 = radius * 2,
                      w = tft.width()  + radius,
                      h = tft.height() + radius;

  // Screen is not cleared for this one -- this is
  // intentional and does not affect the reported time.
  start = micros();
  for (x = 0; x < w; x += r2) {
    for (y = 0; y < h; y += r2) {
      tft.drawCircle(x, y, radius, color);
    }
  }

  return micros() - start;
}

unsigned long testTriangles() {
  unsigned long start;
  int           n, i, cx = tft.width()  / 2 - 1,
                      cy = tft.height() / 2 - 1;

  tft.fillScreen(ILI9341_BLACK);
  n     = min(cx, cy);
  start = micros();
  for (i = 0; i < n; i += 5) {
    tft.drawTriangle(
      cx    , cy - i, // peak
      cx - i, cy + i, // bottom left
      cx + i, cy + i, // bottom right
      tft.color565(0, 0, i));
  }

  return micros() - start;
}

unsigned long testFilledTriangles() {
  unsigned long start, t = 0;
  int           i, cx = tft.width()  / 2 - 1,
                   cy = tft.height() / 2 - 1;

  tft.fillScreen(ILI9341_BLACK);
  start = micros();
  for (i = min(cx, cy); i > 10; i -= 5) {
    start = micros();
    tft.fillTriangle(cx, cy - i, cx - i, cy + i, cx + i, cy + i,
                     tft.color565(0, i, i));
    t += micros() - start;
    tft.drawTriangle(cx, cy - i, cx - i, cy + i, cx + i, cy + i,
                     tft.color565(i, i, 0));
  }

  return t;
}

unsigned long testRoundRects() {
  unsigned long start;
  int           w, i, i2,
                cx = tft.width()  / 2 - 1,
                cy = tft.height() / 2 - 1;

  tft.fillScreen(ILI9341_BLACK);
  w     = min(tft.width(), tft.height());
  start = micros();
  for (i = 0; i < w; i += 6) {
    i2 = i / 2;
    tft.drawRoundRect(cx - i2, cy - i2, i, i, i / 8, tft.color565(i, 0, 0));
  }

  return micros() - start;
}

unsigned long testFilledRoundRects() {
  unsigned long start;
  int           i, i2,
                cx = tft.width()  / 2 - 1,
                cy = tft.height() / 2 - 1;

  tft.fillScreen(ILI9341_BLACK);
  start = micros();
  for (i = min(tft.width(), tft.height()); i > 20; i -= 6) {
    i2 = i / 2;
    tft.fillRoundRect(cx - i2, cy - i2, i, i, i / 8, tft.color565(0, i, 0));
  }

  return micros() - start;
}
