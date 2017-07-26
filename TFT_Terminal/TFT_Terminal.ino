
/*************************************************************
 * Modified by Rambo.co.za 25/7/17. Changed to STM32Duino support using BluePill
 * http://www.instructables.com/id/Arduino-Serial1-UART-scrolling-display-terminal-usi/
  This sketch implements a simple Serial1 receive terminal
  program for monitoring Serial1 debug messages from another
  board.
  
  Connect GND to target board GND
  Connect RX line to TX line of target board
  Make sure the target and terminal have the same baud rate
  and Serial1 stettings!

  The sketch works with the ILI9341 TFT 240x320 display and
  the called up libraries.
  
  The sketch uses the hardware scrolling feature of the
  display. Modification of this sketch may lead to problems
  unless the ILI9341 data sheet has been understood!

  Written by Alan Senior 15th February 2015
  
  BSD license applies, all text above must be included in any
  redistribution
 *************************************************************/

// In most cases characters don't get lost at 9600 baud but
// it is a good idea to increase the Serial1 Rx buffer from 64
// to 512 or 1024 bytes especially if higher baud rates are
// used (this sketch does not need much RAM).
// The method described here works well:
// http://www.hobbytronics.co.uk/arduino-Serial1-buffer-size
//

#include <Adafruit_GFX_AS.h>     // Core graphics library
#include <Adafruit_ILI9341_STM.h> // Hardware-specific library
#include <SPI.h>

// These are the pins used for the UNO, we must use hardware SPI
//#define _sclk 13
//#define _miso 12 // Not used
//#define _mosi 11
#define _cs   PA0    //    pin 3         PA0
#define _dc   PA2    //    pin 5         PA2
#define _rst  PA1    //    pin 4         PA1

//These are the pins I use on the Mega2560, we must use hardware SPI
//#define _sclk 52  // Don't change
//#define _miso 51  // Don't change
//#define _cs   49
//#define _rst  43
//#define _dc   45
#define ILI9341_VSCRDEF 0x33
#define ILI9341_VSCRSADD 0x37


// Must use hardware SPI for speed
Adafruit_ILI9341_STM tft = Adafruit_ILI9341_STM(_cs, _dc, _rst);

// The scrolling area must be a integral multiple of TEXT_HEIGHT
#define TEXT_HEIGHT 16 // Height of text to be printed and scrolled
#define BOT_FIXED_AREA 0 // Number of lines in bottom fixed area (lines counted from bottom of screen)
#define TOP_FIXED_AREA 16 // Number of lines in top fixed area (lines counted from top of screen)

// The initial y coordinate of the top of the scrolling area
uint16_t yStart = TOP_FIXED_AREA;
// yArea must be a integral multiple of TEXT_HEIGHT
uint16_t yArea = 320-TOP_FIXED_AREA-BOT_FIXED_AREA;
// The initial y coordinate of the top of the bottom text line
uint16_t yDraw = 320 - BOT_FIXED_AREA - TEXT_HEIGHT;

// Keep track of the drawing x coordinate
uint16_t xPos = 0;
// For the byte we read from the Serial1 port
byte data = 0;

// A few test varaibles used during debugging
boolean change_colour = 1;
boolean selected = 1;

// We have to blank the top line each time the display is scrolled, but this takes up to 13 milliseconds
// for a full width line, meanwhile the Serial1 buffer may be filling... and overflowing
// We can speed up scrolling of short text lines by just blanking the character we drew
int blank[19]; // We keep all the strings pixel lengths to optimise the speed of the top line blanking

void setup() {
  // Setup the TFT display
  tft.begin();
  tft.setRotation(0);
  tft.fillScreen(ILI9341_BLACK);
  // Setup scroll area
  setupScrollArea(TOP_FIXED_AREA, BOT_FIXED_AREA);
  
  // Setup baud rate and draw top banner
  Serial1.begin(9600);
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLUE);
  tft.fillRect(0,0,240,16, ILI9341_BLUE);
  tft.drawCentreString(" Serial1 Terminal - 9600 baud ",120,0,2);

  // Change colour for scrolling zone
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);

  // Zero the array
  for (byte i = 0; i<18; i++) blank[i]=0;
}


void loop(void) {
  //  These lines change the text colour when the Serial1 buffer is emptied
  //  These are test lines to see if we may be losing characters
  //  Also uncomment the change_colour line below to try them
  //
  //  if (change_colour){
  //  change_colour = 0;
  //  if (selected == 1) {tft.setTextColor(ILI9341_CYAN, ILI9341_BLACK); selected = 0;}
  //  else {tft.setTextColor(ILI9341_MAGENTA, ILI9341_BLACK); selected = 1;}
  //}
  
  while (Serial1.available()) {
    data = Serial1.read();
    if (data == '\r' || xPos>231) {
      xPos = 0;
      yDraw = scroll_line(); // It takes about 13ms to scroll 16 pixel lines
    }
    if (data > 31 && data < 128) {
      xPos += tft.drawChar(data,xPos,yDraw,2);
      blank[(18+(yStart-TOP_FIXED_AREA)/TEXT_HEIGHT)%19]=xPos; // Keep a record of line lengths
    }
    //change_colour = 1; // Line to indicate buffer is being emptied
  }
}

// ##############################################################################################
// Call this function to scroll the display one text line
// ##############################################################################################
int scroll_line() {
  int yTemp = yStart; // Store the old yStart, this is where we draw the next line
  // Use the record of line lengths to optimise the rectangle size we need to erase the top line
  tft.fillRect(0,yStart,blank[(yStart-TOP_FIXED_AREA)/TEXT_HEIGHT],TEXT_HEIGHT, ILI9341_BLACK);

  // Change the top of the scroll area
  yStart+=TEXT_HEIGHT;
  // The value must wrap around as the screen memory is a circular buffer
  if (yStart >= 320 - BOT_FIXED_AREA) yStart = TOP_FIXED_AREA + (yStart - 320 + BOT_FIXED_AREA);
  // Now we can scroll the display
  scrollAddress(yStart);
  return  yTemp;
}

// ##############################################################################################
// Setup a portion of the screen for vertical scrolling
// ##############################################################################################
// We are using a hardware feature of the display, so we can only scroll in portrait orientation
void setupScrollArea(uint16_t TFA, uint16_t BFA) {
  tft.writecommand(ILI9341_VSCRDEF); // Vertical scroll definition
  tft.writedata(TFA >> 8);
  tft.writedata(TFA);
  tft.writedata((320-TFA-BFA)>>8);
  tft.writedata(320-TFA-BFA);
  tft.writedata(BFA >> 8);
  tft.writedata(BFA);
}

// ##############################################################################################
// Setup the vertical scrolling start address
// ##############################################################################################
void scrollAddress(uint16_t VSP) {
  tft.writecommand(ILI9341_VSCRSADD); // Vertical scrolling start address
  tft.writedata(VSP>>8);
  tft.writedata(VSP);
}

