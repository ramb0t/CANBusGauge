#include <Arduino.h>
#include "rcc.h"


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

//CAN stuff
#include <HardwareCAN.h>
#include "Changes.h"

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

// CAN Bus stuff
// Define the values of the identifiers
#define GYRO_ID 0x27
#define JOYSTICK_VALUES_ID 0x5A
#define TANK_LEVEL_ID 0x78
#define MOTOR_CONTROL_ID 0x92

// Limit time to flag a CAN error
#define CAN_TIMEOUT 100
#define CAN_DELAY 10        // ms between two processings of incoming messages
#define CAN_SEND_RATE 100   // ms between two successive sendings

#define ENGINE_COOLANT_TEMP 0x05
#define ENGINE_RPM          0x0C
#define VEHICLE_SPEED       0x0D
#define MAF_SENSOR          0x10
#define O2_VOLTAGE          0x14
#define THROTTLE      0x11

#define PID_REQUEST         0x7DF
#define PID_REPLY     0x7E8


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
// track the message number
byte mno = 0;

// A few test varaibles used during debugging
boolean change_colour = 1;
boolean selected = 1;

// We have to blank the top line each time the display is scrolled, but this takes up to 13 milliseconds
// for a full width line, meanwhile the Serial1 buffer may be filling... and overflowing
// We can speed up scrolling of short text lines by just blanking the character we drew
int blank[19]; // We keep all the strings pixel lengths to optimise the speed of the top line blanking

// Traffic handling data
int CANquietTime ;          // Quiet time counter to detect no activity on CAN bus
bool CANError ;             // Indicates that incoming CAN traffic is missing
int CANsendDivider ;        // Used to send frames once every so many times loop() is called

// Applicaton variables
int Contents[4] ;           // Contents of the four tanks
int JoystickX ;             // Setting of the joystick, X axis
int JoystickY ;             // ... Y axis
int AngularRate ;           // Output of local gyroscope
int Throttle  ;             // Motor control value, produced by some local processing
bool ErreurGyroscope = false ;

// Message structures. Each message has its own identifier. As many such variables should be defined
// as the number of different CAN frames the application has to send. Here, they are two.
CanMsg msgGyroscope ;
CanMsg msgMotorControl ;


// Instanciation of CAN interface
HardwareCAN canBus(CAN1_BASE);

// Note : for the predefined identifiers, please have a look in file can.h

void CANSetup(void)
{
  CAN_STATUS Stat ;

  // Initialize the message structures
  // A CAN structure includes the following fields:
  msgGyroscope.IDE = CAN_ID_STD;          // Indicates a standard identifier ; CAN_ID_EXT would mean this frame uses an extended identifier
  msgGyroscope.RTR = CAN_RTR_DATA;        // Indicated this is a data frame, as opposed to a remote frame (would then be CAN_RTR_REMOTE)
  msgGyroscope.ID = GYRO_ID ;             // Identifier of the frame : 0-2047 (0-0x3ff) for standard idenfiers; 0-0x1fffffff for extended identifiers
  msgGyroscope.DLC = 3;                   // Number of data bytes to follow
  msgGyroscope.Data[0] = 0x0;             // Data bytes, there can be 0 to 8 bytes.
  msgGyroscope.Data[1] = 0x0;
  msgGyroscope.Data[2] = 0x0;

  msgMotorControl.IDE = CAN_ID_STD;
  msgMotorControl.RTR = CAN_RTR_DATA;
  msgMotorControl.ID = MOTOR_CONTROL_ID ;
  msgMotorControl.DLC = 2;
  msgMotorControl.Data[0] = 0x0;
  msgMotorControl.Data[1] = 0x0;

  // Initialize CAN module
  canBus.map(CAN_GPIO_PB8_PB9);       // This setting is already wired in the Olimexino-STM32 board
  Serial1.println("CAN Rx on PB8, Tx PB9");
  Stat = canBus.begin(CAN_SPEED_500, CAN_MODE_NORMAL);    // Other speeds go from 125 kbps to 1000 kbps. CAN allows even more choices.
  Serial1.println("500khz bus speed");

  canBus.filter(0, 0, 0);
  canBus.set_irq_mode();              // Use irq mode (recommended), so the handling of incoming messages
                                      // will be performed at ease in a task or in the loop. The software fifo is 16 cells long,
                                      // allowing at least 15 ms before processing the fifo is needed at 125 kbps
  Stat = canBus.status();
  if (Stat != CAN_OK)
     Serial1.println("CAN Init failed :(") ;   // Initialization failed
  else
     Serial1.println("CAN Init success! :D");
}

// Send one frame. Parameter is a pointer to a frame structure (above), that has previously been updated with data.
// If no mailbox is available, wait until one becomes empty. There are 3 mailboxes.
CAN_TX_MBX CANsend(CanMsg *pmsg)
{
  CAN_TX_MBX mbx;

  do
  {
    mbx = canBus.send(pmsg) ;
#ifdef USE_MULTITASK
    vTaskDelay( 1 ) ;                 // Infinite loops are not multitasking-friendly
#endif
  }
  while(mbx == CAN_TX_NO_MBX) ;       // Waiting outbound frames will eventually be sent, unless there is a CAN bus failure.
  return mbx ;
}

// Process incoming messages
// Note : frames are not fully checked for correctness: DLC value is not checked, neither are the IDE and RTR fields. However, the data is guaranteed to be corrrect.
void ProcessMessages(void)
{

  //Serial1.println("Processing CAN messages");
  int Pr = 0 ;
  int i ;

  CanMsg *r_msg;

  // Loop for every message in the fifo
  while ((r_msg = canBus.recv()) != NULL)
  {
    CANquietTime = 0 ;              // Reset at each received frame
    CANError = false ;              // Clear CAN silence error

    // Print to serial
    //Serial1.println("ID: ");
    //Serial1.print(r_msg->ID,HEX);
//    Serial1.print(", ");
//    Serial1.print("Data: ");
//    //Serial1.print((int)r_msg->Data.length,DEC);
//    for(int i=0;i<7;i++)
//     {
//       Serial1.print(r_msg->Data[i],HEX);
//       Serial1.print(" ");
//     }
//    Serial1.println("");

    // Print to TFT

    xPos = 0;
    yDraw = scroll_line(); // It takes about 13ms to scroll 16 pixel lines
    tft.setCursor(xPos, yDraw);
    tft.print(mno);
    mno++;
    tft.print("ID: ");
    tft.print(r_msg->ID,HEX);
    tft.print(", ");
    tft.print("Data: ");
    //Serial1.print((int)r_msg->Data.length,DEC);
    for(int i=0;i<8;i++)
     {
       tft.print(r_msg->Data[i],HEX);
       tft.print(" ");
     }

    switch ( r_msg->ID )
    {
//      case TANK_LEVEL_ID :                  // This frame contains four 16-bit words, little endian coded
//        for ( i = 0 ; i < 4 ; i++ )
//          Contents[i] = (int)r_msg->Data[2*i] | ((int)r_msg->Data[(2*i)+1]) << 8 ;
//        break ;
//
//      case JOYSTICK_VALUES_ID :             // This frame contains two 16-bit words, little endian coded
//        Pr = (int)r_msg->Data[0] ;
//        Pr |= (int)r_msg->Data[1] << 8 ;
//        JoystickX = Pr ;
//
//        Pr = (int)r_msg->Data[2] ;
//        Pr |= (int)r_msg->Data[3] << 8 ;
//        JoystickY = Pr ;
//        break ;

//      case PID_REPLY :
//            if(r_msg->Data[2] == 0x0c){
//              xPos = 0;
//              yDraw = scroll_line(); // It takes about 13ms to scroll 16 pixel lines
//              tft.setCursor(xPos, yDraw);
//              tft.print(mno);
//              mno++;
//              tft.print("ID: ");
//              tft.print(r_msg->ID,HEX);
//              tft.print(", ");
//              tft.print("Data: ");
//              //Serial1.print((int)r_msg->Data.length,DEC);
//              for(int i=0;i<8;i++)
//               {
//                 tft.print(r_msg->Data[i],HEX);
//                 tft.print(" ");
//               }
//            }
//        break;

      default :                     // Any frame with a different identifier is ignored
        break ;
    }

    canBus.free();                          // Remove processed message from buffer, whatever the identifier
#ifdef USE_MULTITASK
    vTaskDelay( 1 ) ;                       // Infinite loops are not multitasking-friendly
#endif
  }
}

// Send messages
// Prepare and send 2 frames containing the value of process variables
// Sending all frames at once is a choice; they could be sent separately, at different times and rates.
void SendCANmessages(void)
{
  //Serial1.println("Sending CAN Messages.. ");
  CanMsg message;
  //float engine_data;
  //int timeout = 0;
  //char message_ok = 0;
  // Prepair message
  //message.id = PID_REQUEST;
  message.IDE = CAN_ID_STD;          // Indicates a standard identifier ; CAN_ID_EXT would mean this frame uses an extended identifier
  message.RTR = CAN_RTR_DATA;        // Indicated this is a data frame, as opposed to a remote frame (would then be CAN_RTR_REMOTE)
  message.ID = 0x7DF;
  message.DLC = 8;                   // Number of data bytes to follow
  message.Data[0] = 0x02;
  message.Data[1] = 0x01;
  message.Data[2] = 0x0C;  //RPM
  message.Data[3] = 0x00;
  message.Data[4] = 0x00;
  message.Data[5] = 0x00;
  message.Data[6] = 0x00;
  message.Data[7] = 0x00;
  CANsend(&message) ;      // Send this frame

}

void setup() {
  // Setup the TFT display
  tft.begin();
  tft.setRotation(0);
  tft.fillScreen(ILI9341_BLACK);
  // Setup scroll area
  setupScrollArea(TOP_FIXED_AREA, BOT_FIXED_AREA);

  // Setup baud rate and draw top banner
  Serial1.begin(115200);
  Serial1.println("Starting Up!");
  // put your setup code here, to run once:
  CANSetup() ;        // Initialize the CAN module and prepare the message structures.
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLUE);
  tft.fillRect(0,0,240,16, ILI9341_BLUE);
  tft.drawCentreString(" Serial1 Terminal - 115200 baud ",120,0,2);

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

  // Process incoming messages periodically (should be often enough to avoid overflowing the fifo)
  ProcessMessages() ;          // Process all incoming messages, update local variables accordingly

  // This is an example of timeout management. Here it is global to all received frames;
  // it could be on a frame by frame basis, with as many control variables as the number of frames.
  CANquietTime++ ;
  if ( CANquietTime > CAN_TIMEOUT )
  {
    CANquietTime = CAN_TIMEOUT + 1 ;            // To prevent overflowing this variable if silence prolongs...
    CANError = true ;                           // Flag CAN silence error. Will be cleared at first frame received
  }

  // Send messages containing variables to publish. Sent less frequently than the processing of incoming frames (here, every 200 ms)
  CANsendDivider-- ;
  if ( CANsendDivider < 0 )
  {
    CANsendDivider = CAN_SEND_RATE / CAN_DELAY ;
    SendCANmessages() ;
  }
  delay(CAN_DELAY) ;    // The delay must not be greater than the time to overflow the incoming fifo (here about 15 ms)

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
