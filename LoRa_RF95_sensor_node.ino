#include <SPI.h>                  //Arduino serial interface with the LoRa module and SD logger
#include <RHReliableDatagram.h>   //Manager library used for addressed, unreliable messages
#include <RH_RF95.h>              //Library used to send messages using LoRa
#include <SoftwareSerial.h>       //Software serial library for interface with Atlas Scientific sensors
#include <OneWire.h>              //OneWire interface for DS18B20 temperature sensor
#include <DallasTemperature.h>    //Library for DS18B20 temperature sensor
#include <stdio.h>

/*
  #define s0 7
  #define s1 6
  #define temp 2
*/

#define RF_FREQUENCY  433.00
#define RF_GATEWAY_ID 0
#define RF_NODE_ID    1
//#define RF_NODE_ID    11

RH_RF95 rf95d;                        //Singleton instance of the radio driver
RHReliableDatagram rf95m(rf95d, RF_NODE_ID);  //This class manages message delivery and reception

//SoftwareSerial altSerial(8, 9);     //Name the software serial library altSerial (this cannot be omitted)
//OneWire ds(temp);                   // on digital pin 2
//DallasTemperature tsens(&ds);     // DallasTemperature object for DS18B20 sensor
//DeviceAddress waterTemp;

//char computerdata[20];            //A 20 byte character array to hold incoming data from a pc/mac/other
//char sensordata[30];                //A 30 byte character array to hold incoming data from the sensors
//byte computer_bytes_received=0;   //We need to know how many characters bytes have been received
//byte sensor_bytes_received = 0;     //We need to know how many characters bytes have been received

//char *channel;                      //Char pointer used in string parsing
//char *cmd;                          //Char pointer used in string parsing

struct dataStruct {     //stores the sensor values in a struct for easier sending and receiving via LoRa
  float temp0, temp1, diox0, diox1, cdty0, cdty1;
    // uint8_t _temp0[4], _temp1[4], _diox0[4], _diox1[4], _cdty0[4], _cdty1[4];
} nodeData;

void setup() {
  Serial.begin(9600);               //Set the hardware serial port to 9600
  while (!Serial);                  //Wait for serial port to be available
//  altSerial.begin(9600);          //Set the software serial port to 9600
//  tsens.begin();                  //Start DallasTemperature object

  /*--Initializing LoRa module--*/
  if (rf95m.init()) {
    //Adjust Frequency
    rf95d.setFrequency(RF_FREQUENCY);
  
    //Adjust Power to 23 dBm
    rf95d.setTxPower(23, false);
  
    // This is our Node ID
    rf95m.setThisAddress(RF_NODE_ID);
    rf95m.setHeaderFrom(RF_NODE_ID);
  
    // Where we're sending packet
    rf95m.setHeaderTo(RF_GATEWAY_ID);
    //rf95m.setRetries(7); 
    }

  //Setting up the sensor interfaces
  //analogReference(DEFAULT);         //Default at 5.0V
  /*
    pinMode(s1, OUTPUT);              //Set the digital pin as output for multiplex selection (s1)
    pinMode(s0, OUTPUT);              //Set the digital pin as output for multiplex selection (s0)
    altSerial.begin(9600);            //Set the soft serial port to 9600
    Serial.print("Clearing trash data: ");
    Serial.print(tsens.getTempCByIndex(0)); Serial.print(",");
    Serial.print(getAtlasDO()); Serial.print(",");
    Serial.print(getAtlasCdty());
  */
}

uint8_t rx_buf[RH_RF95_MAX_MESSAGE_LEN], tx_buf[RH_RF95_MAX_MESSAGE_LEN];

void loop(void) {
//  Sample data for testing
//  float temp0 = 22.57, temp1 = -999.99, diox0 = 5.40, diox1 = -999.99, cdty0 = 338.53, cdty1 = -999.99; // sample sensor data
  float _temp0 = 24.68, _temp1 = -999.99, _diox0 = 7.61, _diox1 = -999.99, _cdty0 = 268.53, _cdty1 = -999.99; // sample sensor data
  nodeData.temp0 = _temp0;
  nodeData.temp1 = _temp1;
  nodeData.diox0 = _diox0;
  nodeData.diox1 = _diox1;
  nodeData.cdty0 = _cdty0;
  nodeData.cdty1 = _cdty1;
  
  // store sensor data to struct for sending to RF95 server
//  memcpy(nodeData._temp0, &temp0, sizeof(temp0));
//  memcpy(nodeData.temp1, &_temp1, sizeof(temp1));
//  memcpy(nodeData.diox0, &_diox0, sizeof(_diox0));
//  memcpy(nodeData.diox1, &_diox1, sizeof(_diox1));
//  memcpy(nodeData.cdty0, &_cdty0, sizeof(_cdty0));
//  memcpy(nodeData.cdty1, &_cdty1, sizeof(_cdty1));
  if (rf95m.available())
  {
    uint8_t len = sizeof(rx_buf);
    uint8_t from;
    if (rf95m.recvfrom(rx_buf, &len, &from))
    { //If trigger is received from server, send recent data or accumulated data
      if (rx_buf[0] == 83)
      { //If trigger received is "S", get latest data from sensors and send them
//        nodeData.temp0 = tsens.getTempCByIndex(0);
//        nodeData.temp1 = -999.99;
//        nodeData.diox0 = getAtlasDO();
//        nodeData.diox1 = -999.99;
//        nodeData.cdty0 = getAtlasCdty();
//        nodeData.cdty1 = -999.99;
        memcpy(tx_buf, &nodeData, sizeof(nodeData));
        rf95m.sendtoWait(tx_buf, sizeof(nodeData), RF_GATEWAY_ID);
        rf95m.waitPacketSent();
      }
    }
  }
}

/*
float getAtlasCdty() {
  char *sensorval;                     //Char pointer used to isolate sensor value
  channel = '1';
  open_channel();
  altSerial.print("R\r");                        //After we send the command we send a carriage return <CR>
  delay(1000);

  if (altSerial.available() > 0) {                 //If data has been transmitted from an Atlas Scientific device
    sensor_bytes_received = altSerial.readBytes(sensordata,30);
    sensordata[sensor_bytes_received] = 0;         //we add a 0 to the spot in the array just after the last character we received. This will stop us from transmitting incorrect data that may have been left in the buffer
    sensorval = strtok(sensordata,"\r");
    return atof(sensorval);                    //let’s transmit the data received from the Atlas Scientific device to the serial monitor
  }
}

float getAtlasDO() {
  char *sensorval;
  channel = '2';
  open_channel();
  altSerial.print("R\r");                        //After we send the command we send a carriage return <CR>
  delay(1000);

  if (altSerial.available() > 0) {                 //If data has been transmitted from an Atlas Scientific device
    sensor_bytes_received = altSerial.readBytes(sensordata, 30); //we read the data sent from the Atlas Scientific device until we see a <CR>. We also count how many character have been received
    sensordata[sensor_bytes_received] = 0;         //we add a 0 to the spot in the array just after the last character we received. This will stop us from transmitting incorrect data that may have been left in the buffer
    sensorval = strtok(sensordata,"\r");
    return atof(sensorval);                    //let’s transmit the data received from the Atlas Scientific device to the serial monitor
  }
}

void open_channel() {                                 //This function controls what UART port is opened.

  switch (*channel) {                              //Looking to see what channel to open

    case '0':                                      //If channel==0 then we open channel 0
      digitalWrite(s0, LOW);                       //S0 and S1 control what channel opens
      digitalWrite(s1, LOW);                       //S0 and S1 control what channel opens
      break;                                         //Exit switch case

    case '1':
      digitalWrite(s0, HIGH);
      digitalWrite(s1, LOW);
      break;

    case '2':
      digitalWrite(s0, LOW);
      digitalWrite(s1, HIGH);
      break;

    case '3':
      digitalWrite(s0, HIGH);
      digitalWrite(s1, HIGH);
      break;
  }
}
*/
