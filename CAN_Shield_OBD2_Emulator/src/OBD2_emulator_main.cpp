/****************************************************************************
Based on CAN Write Demo for the SparkFun CAN Bus Shield.
Modified to respond to some OBD2 queries by Ramb0t

Written by Stephen McCoy.
Original tutorial available here: http://www.instructables.com/id/CAN-Bus-Sniffing-and-Broadcasting-with-Arduino
Used with permission 2016. License CC By SA.

Distributed as-is; no warranty is given.
*************************************************************************/

#include <Arduino.h>
#include <Canbus.h>
#include <defaults.h>
#include <global.h>
#include <mcp2515.h>
#include <mcp2515_defs.h>

//********************************Setup Loop*********************************//

void setup() {
  Serial.begin(115200); // For debug use
  Serial.println("CAN Read - Testing receival of CAN Bus message");
  delay(500);

  if(Canbus.init(CANSPEED_500))  //Initialise MCP2515 CAN controller at the specified speed
    Serial.println("CAN Module Init ok");
  else
    Serial.println("Can't init CAN Bus");

  // Get some random seed
  pinMode(A0, INPUT);
  randomSeed(analogRead(0));
}

//********************************Main Loop*********************************//
uint8_t count;
void loop(){
  tCAN message;
  if (mcp2515_check_message()) // Do we have a message?
	{
    if (mcp2515_get_message(&message)) // Get the message
	  {
      // Print out the Message
      Serial.print("ID: ");
      Serial.print(message.id,HEX);
      Serial.print(", ");
      Serial.print("Data: ");
      Serial.print(message.header.length,DEC);
      for(int i=0;i<message.header.length;i++)
      {
        Serial.print(message.data[i],HEX);
        Serial.print(" ");
      }
      Serial.println("");


      //****************//
      //OBD2 Responses based on https://en.wikipedia.org/wiki/OBD-II_PIDs
      // Boost
      if(message.id == 0x7DF and message.data[2] == 0x0B)
      {
        tCAN message0;

        message0.id = 0x7E8; //formatted in HEX
        message0.header.rtr = 0;
        message0.header.length = 4; //formatted in DEC
        message0.data[0] = 0x03;
        message0.data[1] = 0x41;
        message0.data[2] = 0x0B;
        message0.data[3] = map(analogRead(A0), 0, 1023, 0, 255); //formatted in HEX

        message0.data[4] = 0x00;
        message0.data[5] = 0x40;
        message0.data[6] = 0x00;
        message0.data[7] = 0x00;

        mcp2515_bit_modify(CANCTRL, (1<<REQOP2)|(1<<REQOP1)|(1<<REQOP0), 0);
        mcp2515_send_message(&message0);
      }
      // Timing
      if(message.id == 0x7DF and message.data[2] == 0x0E)
      {
        tCAN message0;

        message0.id = 0x7E8; //formatted in HEX
        message0.header.rtr = 0;
        message0.header.length = 4; //formatted in DEC
        message0.data[0] = 0x03;
        message0.data[1] = 0x41;
        message0.data[2] = 0x0E;
        message0.data[3] = random(255); //formatted in HEX

        message0.data[4] = 0x00;
        message0.data[5] = 0x40;
        message0.data[6] = 0x00;
        message0.data[7] = 0x00;

        mcp2515_bit_modify(CANCTRL, (1<<REQOP2)|(1<<REQOP1)|(1<<REQOP0), 0);
        mcp2515_send_message(&message0);
      }
      // Throttle
      if(message.id == 0x7DF and message.data[2] == 0x11)
      {
        tCAN message0;

        message0.id = 0x7E8; //formatted in HEX
        message0.header.rtr = 0;
        message0.header.length = 4; //formatted in DEC
        message0.data[0] = 0x03;
        message0.data[1] = 0x41;
        message0.data[2] = 0x11;
        message0.data[3] = random(100);//formatted in HEX

        message0.data[4] = 0x00;
        message0.data[5] = 0x40;
        message0.data[6] = 0x00;
        message0.data[7] = 0x00;

        mcp2515_bit_modify(CANCTRL, (1<<REQOP2)|(1<<REQOP1)|(1<<REQOP0), 0);
        mcp2515_send_message(&message0);
      }
    } // get message
  } // check for message
} // main loop
