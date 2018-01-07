/*
 * Modified by Rambo.co.za 25/7/17. Changed to STM32Duino support using BluePill 
 TFT ILI9340 Serial Terminal Sketch by M. Ray Burnette ==> Public Domain https://www.hackster.io/rayburne/color-glcd-terminal-26-ch-x-15-ln-83fa7d
 04/20/2014
 Arduino 1.0.5r2    Board=Arduino Micro (32U4) Profile= Sparkfun Pro Micro 5V 16MHz
 Binary sketch size: 15,756 bytes (of a 28,672 byte maximum)
 Program:   15734 bytes (.text + .data + .bootloader)
 Data:        809 bytes (.data + .bss + .noinit)
 
 Primary Color definitions for TFT display: int_16
 ILI9340_BLACK   0x0000, ILI9340_BLUE    0x001F, ILI9340_RED     0xF800, ILI9340_GREEN   0x07E0
 ILI9340_CYAN    0x07FF, ILI9340_MAGENTA 0xF81F, ILI9340_YELLOW  0xFFE0, ILI9340_WHITE   0xFFFF
 */
// See tab/file License.h for use/rights information regarding Adafruit library

#include <SPI.h>
//#include <Serial1.h> // hardware serial Rx, Tx on 32U4 w/ LUFA
#include <Adafruit_GFX_AS.h>
#include <Adafruit_ILI9341_STM.h>
//#include <Streaming.h>

// Pinouts for ILI9340 2.2" 240x320 TFT LCD SPI interface (SD card not attached)
//      Pro Mini    LCD Display    UNO pins
//#define _sclk 15    // J2 pin 7        13
//#define _miso 14    //    pin 9        12
//#define _mosi 16    //    pin 6        11
#define _cs   PA0    //    pin 3         PA0
#define _dc   PA2    //    pin 5         PA2
#define _rst  PA1    //    pin 4         PA1

// Do Note use PROGMEM with Pro Micro ...  Bootloader lockup Reason unknown
String sBlank = "                          " ;  // 26 characters == 1 line
boolean lFlag = true ;
boolean lDebug = false ;
const int yCord[] = {
  0, 16, 32, 48, 64, 80, 96, 112, 128, 144, 160, 176, 192, 208, 224} 
;
byte index ;

// Using software SPI is really not suggested, its incredibly slow... Use hardware SPI
// Adafruit_ILI9340 tft = Adafruit_ILI9340(_cs, _dc, _mosi, _sclk, _rst, _miso);
Adafruit_ILI9341 lcd = Adafruit_ILI9341(_cs, _dc, _rst);

void setup()  
{
  lcd.begin() ;        // 2.2" GLCD SPI 240x320 used in LANDSCAPE
  lcd.setRotation(3);  // landscape with SPI conn J2 to LEFT side.
  lcd.fillScreen(ILI9340_BLACK);
  lcd.setCursor(0, 0);
  lcd.setTextColor(ILI9340_YELLOW) ;
  lcd.setTextSize(3);     // Medium 17 char / line  10Pixels Wide x 25 Pixels / line

  analogReference(DEFAULT);

  pinMode( 4, INPUT) ;     // lDebug Low = True
  digitalWrite( 4, HIGH);  // turn on pullup resistor
  pinMode( 6, INPUT) ;     // BAUD hi = 9600 lo = 4800 for setting common GPS rates
  digitalWrite( 6, HIGH);  // turn on pullup resistor
  delay (100) ;

  if (! digitalRead(4) ) lDebug = true;

  if( digitalRead(6) ) {
    Serial1.begin(9600) ;   // 1st serial port because 'Serial' translates to USB comm:
  } 
  else {
    Serial1.begin(4800) ;   // 1st serial port because 'Serial' translates to USB comm:
  }

  if(lDebug) Serial1 << "Serial Monitor \n \r" ;  // physical port Tx
  lcd.setCursor(0, 0) ;
  lcd.setTextColor(ILI9340_BLUE) ;   lcd << "S" ;
  lcd.setTextColor(ILI9340_RED) ;    lcd << "e" ;
  lcd.setTextColor(ILI9340_GREEN) ;  lcd << "r" ;
  lcd.setTextColor(ILI9340_YELLOW) ; lcd << "i" ;
  lcd.setTextColor(ILI9340_WHITE) ;  lcd << "a" ;
  lcd.setTextColor(ILI9340_CYAN) ;   lcd << "l " ;
  lcd.setTextColor(ILI9340_MAGENTA) ;lcd << "M" ;
  lcd.setTextColor(ILI9340_BLUE) ;   lcd << "o" ;
  lcd.setTextColor(ILI9340_BLUE) ;   lcd << "n" ;
  lcd.setTextColor(ILI9340_RED) ;    lcd << "i" ;
  lcd.setTextColor(ILI9340_GREEN) ;  lcd << "t" ;
  lcd.setTextColor(ILI9340_YELLOW) ; lcd << "o" ;
  lcd.setTextColor(ILI9340_WHITE) ;  lcd << "r " ;
  lcd.setTextColor(ILI9340_CYAN) ;   lcd << "b" ;
  lcd.setTextColor(ILI9340_MAGENTA) ;lcd << "y \r \n" ;
  lcd.setTextColor(ILI9340_BLUE) ;   lcd << "R" ;
  lcd.setTextColor(ILI9340_CYAN) ;   lcd << "a" ;
  lcd.setTextColor(ILI9340_MAGENTA) ;lcd << "y " ;
  lcd.setTextColor(ILI9340_BLUE) ;   lcd << "B" ;
  lcd.setTextColor(ILI9340_RED) ;    lcd << "u" ;
  lcd.setTextColor(ILI9340_GREEN) ;  lcd << "r" ;
  lcd.setTextColor(ILI9340_YELLOW) ; lcd << "n" ;
  lcd.setTextColor(ILI9340_WHITE) ;  lcd << "e" ;
  lcd.setTextColor(ILI9340_CYAN) ;   lcd << "t" ;
  lcd.setTextColor(ILI9340_MAGENTA) ;lcd << "t" ;
  lcd.setTextColor(ILI9340_BLUE) ;   lcd << "e" ;
  lcd.setCursor(0, 224) ;
  lcd.setTextSize(2) ;

  lcd.print("April 2014  Version 1.00") ;
  delay(2000) ;  // credits
  if(lDebug)  Serial1 << "Color Monitor coming online \n \r" ;
  lcd.fillScreen(ILI9340_BLACK) ;
  lcd.setTextColor(ILI9340_GREEN, ILI9340_BLACK) ;
  lcd.setTextSize(2) ;
  lcd.setCursor(0, 0) ;
}

int nRow   = 0 ;  // 0 - 9
int nCol   = 0 ;  // 0 - 16
int nLCD   = 0 ;  // 1 - 390 arranged as 15 lines x 26 characters
int nLn    = 1 ;  // current line, 1-based


void loop()
{
  if ( ! lFlag )              // new line roll-over cleanup
  {
    nCol = 0 ;                // align character pointer to "x" home
    index = int(nLCD / 26) ;  // character pointer to current-line to graphic "y" lookup of home character
    nRow = yCord[ index ]  ;  // [0 - 14] ==> lines 1 - 15
    if(lDebug)
    {
      Serial1 << "Inside flag routine! \n \r" ;
      Serial1 << "nCol / nRow / nLCD = " << nCol << " / " << nRow << " / " << nLCD << "\n \r" ;
    }
    lcd.setCursor (nCol, nRow) ;  // restore cursor to home character of current line
    lcd.print( sBlank ) ;         // clear line
    //tft.drawLine( x1,  y1,  x2,   y2, color);
    lcd.drawLine(    0,  (nRow + 7),  310,    (nRow + 7), ILI9340_YELLOW);
    lcd.setCursor (nCol, nRow) ;  // restore cursor to home character of current line
    lFlag = true ;                // reset flag for normal operation
  }

  // read data from serial port
  if (Serial1.available())
  {
    char c = Serial1.read() ;
    if (c)    // non-Null
    {
      if( c > 31 && c < 127)
      {
        lcd.print(c) ;  // keep non-printables off LCD
        ++nLCD ;        // increment character count
      }

      if( c == 13 )            // carriage return?
      {
        c = 0 ;
        lcd.print( sBlank ) ;         // clear line from current position, may wrap
        nRow = (nLCD / 26) + 1 ;      // next row (row == 0, 1, ... 14)
        if( nRow > 15 ) nRow = 0 ;    // maintain boundary
        index = nRow ;                // array index for graphic "Y" position of first character
        nCol = 0 ;                    // "X" Graphic coordinate
        nRow = yCord[ index ]  ;      // Graphic "Y" coordinate via lookup
        nLCD = index * 26  ;          // 26 characters per line (line = nRow +1)
        lcd.setCursor (nCol, nRow) ;  // restore cursor to home character of current line
        //lcd.print( sBlank ) ;         // clear NEW line
        lcd.setCursor (nCol, nRow) ;  // restore cursor to home character of current line
        if(lDebug)
        {
          Serial1 << "Return Pressed!  " << "nCol / nRow / nLCD == " ;
          Serial1 << nCol << " / " << nRow << " / " << nLCD << "\n \r" ;
        }
      }

      if( nLCD % 26 == 0 ) // just printed last character on line?
      {
        if(lDebug) Serial1 << "Last character of line printed!  " ;

        if(  nLCD == 390)  // last character of last line?
        {
          if(lDebug) Serial1 << "End of display! \n \r" ;
          nLCD = 0 ;  
          c = 0 ;
          lFlag = false ;
        }
        else 
        {
          lFlag = false ;  // activate clean-up routine
          //nLCD++  ;   // increment to 1st position next line
          lcd.print( sBlank ) ; // clear end of line and some of next
          if(lDebug) Serial1 << "nCol / nRow / nLCD == " << nCol << " / " << nRow << " / " << nRow << "\n \r" ;
        }
      }  
    }
  }
}






