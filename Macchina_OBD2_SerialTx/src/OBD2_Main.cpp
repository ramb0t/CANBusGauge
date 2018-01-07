#include <Arduino.h>

#include <OBD2.h>
#include <DueTimer.h>

//Serial Tx using EasyTransfer lib
#include <EasyTransfer.h>
/********************************************************************
This example is built upon the CANAcquisition class and the OBDParmameter class using 11bit (non-extended) OBD2 ID's

This example shows how to set up periodic data acquisition of OBD2 paramters based upon
standard PID's. If you'd like to add another paramter,simply copy one of the definitions and modify it accordingly.
You may also need to add a new PID to the "OBD_PID" enum in the header file.

Note as of 8/24/15 this has only been tested on one Toyota vehicle!
As of July 24, 2017 it has also been tested on a Buick Enclave. Works fine!
/********************************************************************/

//create the CANport acqisition schedulers
cAcquireCAN CANport0(CAN_PORT_0);

/***** DEFINITIONS FOR OBD MESSAGES ON CAN PORT 0, see https://en.wikipedia.org/wiki/OBD-II_PIDs to add your own ***************/
//char _name[10], char _units[10], OBD_PID pid,  uint8_t OBD_PID_SIZE size, bool _signed, OBD_MODE_REQ mode, float32 slope, float32 offset, cAcquireCAN *, extended ID;

  //cOBDParameter OBD_Speed(      "Speed "        , " KPH"		,  SPEED       , _8BITS,   false,   CURRENT,  1,      0,  &CANport0, false);
  //cOBDParameter OBD_EngineSpeed("Engine Speed " , " RPM"		,  ENGINE_RPM  , _16BITS,  false,   CURRENT,  0.25,   0,  &CANport0, false);
  //cOBDParameter OBD_Throttle(   "Throttle "     , " %"  		,  THROTTLE_POS, _8BITS,   false,   CURRENT,  0.3922, 0,  &CANport0, false);
  cOBDParameter OBD_Timing(   "Timing "     , " deg"      ,  TIMING_ADV, _8BITS,   false,   CURRENT,  0.5, 0,  &CANport0, false);
  //cOBDParameter OBD_Coolant(    "Coolant "      , " C"  		,  COOLANT_TEMP, _8BITS,   false ,  CURRENT,  1,    -40,  &CANport0, false);
  //cOBDParameter OBD_EngineLoad( "Load "         , " %"  		,  ENGINE_LOAD , _8BITS,   false,   CURRENT,  0.3922, 0,  &CANport0, false);
  //cOBDParameter OBD_MAF(        "MAF "          , " grams/s",  ENGINE_MAF  , _16BITS,  false,   CURRENT,  0.01,   0,  &CANport0, false);
  cOBDParameter OBD_MAP(        "MAP "          , " kPa"    ,  ENGINE_MAP  , _8BITS,   false,   CURRENT,  1,      0,  &CANport0, false);
  //cOBDParameter OBD_IAT(        "IAT "          , " C"  		,  ENGINE_IAT  , _8BITS,   false ,  CURRENT,  1,    -40,  &CANport0, false);


// Serial Tx Struct setup
//create object
EasyTransfer ET;

struct SEND_DATA_STRUCTURE{
  //put your variable definitions here for the data you want to send
  //THIS MUST BE EXACTLY THE SAME ON THE OTHER ARDUINO
  uint8_t boost;
  uint8_t throttle;
  uint8_t injector;
  uint8_t timing;
};

//Function prototypes
void PrintScreen();

//give a name to the group of data
SEND_DATA_STRUCTURE mydata;

void setup()
{
  pinMode(PS_BUCK, OUTPUT);
  digitalWrite(PS_BUCK, HIGH);

	//output pin that can be used for debugging purposes
	pinMode(RGB_GREEN, OUTPUT);
  pinMode(GPIO1    , OUTPUT);
  pinMode(I_SENSE_EN, OUTPUT);
  digitalWrite(GPIO1 , HIGH);
  digitalWrite(I_SENSE_EN, HIGH);

	//start serial port
	SerialUSB.begin(115200);
  delay(500); //allow USB time to settle

  //start the second serial
  Serial3.begin(115200);

	//debugging message for monitor to indicate CPU resets are occuring
	SerialUSB.println("System Reset");
  Serial3.println("System Reset");

  //start the library, pass in the data details and the name of the serial port. Can be Serial, Serial1, Serial2, etc.
  ET.begin(details(mydata), &Serial3);

  //start CAN ports,  enable interrupts and RX masks, set the baud rate here
	CANport0.initialize(_500K);

  //set up the transmission/reception of messages to occur at 500Hz (2mS) timer interrupt
  Timer3.attachInterrupt(PrintScreen).setFrequency(20).start();

}


UINT8 i;
UINT32 maxTime;

void loop()
{
  CANport0.run(POLLING);

}

//this is our timer interrupt handler, called at XmS interval
void PrintScreen()
{
  digitalWrite(RGB_GREEN, LOW);

	//print out our latest OBDII data
//	SerialUSB.print(OBD_Speed.getName());
//	SerialUSB.print(OBD_Speed.getData());
//	SerialUSB.println(OBD_Speed.getUnits());

//    SerialUSB.print(OBD_EngineSpeed.getName());
//	SerialUSB.print(OBD_EngineSpeed.getData());
//	SerialUSB.println(OBD_EngineSpeed.getUnits());
//
	//SerialUSB.print(OBD_Throttle.getName());
	//SerialUSB.print(OBD_Throttle.getData());
	//SerialUSB.println(OBD_Throttle.getUnits());
  //mydata.throttle = OBD_Throttle.getIntData();

  //SerialUSB.print(OBD_Timing.getName());
  //SerialUSB.print(OBD_Timing.getData());
  //SerialUSB.println(OBD_Timing.getUnits());
  mydata.timing = OBD_Timing.getIntData();
//
//	SerialUSB.print(OBD_Coolant.getName());
//	SerialUSB.print(OBD_Coolant.getData());
//	SerialUSB.println(OBD_Coolant.getUnits());
//
//	SerialUSB.print(OBD_EngineLoad.getName());
//	SerialUSB.print(OBD_EngineLoad.getData());
//	SerialUSB.println(OBD_EngineLoad.getUnits());
//
//	SerialUSB.print(OBD_MAF.getName());
//	SerialUSB.print(OBD_MAF.getData());
//	SerialUSB.println(OBD_MAF.getUnits());

  SerialUSB.print(OBD_MAP.getName());
  SerialUSB.print(OBD_MAP.getData());
  SerialUSB.println(OBD_MAP.getUnits());
  mydata.boost = OBD_MAP.getIntData();


//
//	SerialUSB.print(OBD_IAT.getName());
//	SerialUSB.print(OBD_IAT.getData());
//	SerialUSB.println(OBD_IAT.getUnits());

  SerialUSB.println();

  // send data to display
  digitalWrite(RGB_BLUE, LOW);
  ET.sendData();
  digitalWrite(RGB_BLUE, HIGH);

  digitalWrite(RGB_GREEN, HIGH);
}
