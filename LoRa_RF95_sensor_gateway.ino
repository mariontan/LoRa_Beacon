#include <SPI.h>            //Arduino serial interface with the LoRa module and SD logger
#include <RHReliableDatagram.h>     //Manager library used for addressed, unreliable messages
#include <RH_RF95.h>        //Library used to send messages using LoRa
#include <stdio.h>

#define RF_FREQUENCY  868.00
#define RF_GATEWAY_ID 0

RH_RF95 rf95d;                        //Singleton instance of the radio driver
RHReliableDatagram rf95m(rf95d, RF_GATEWAY_ID);  //This class manages message delivery and reception

struct dataStruct {     //stores the sensor values in a struct for easier sending and receiving via LoRa
  float temp, diox, cdty;
} nodeData;

void setup() {
  Serial.begin(9600);               //Set the hardware serial port to 9600
  while (!Serial);                  //Wait for serial port to be available
  //  altSerial.begin(9600);            //Set the software serial port to 9600
  /*--Initializing LoRa module--*/

  if (rf95m.init()) {
    //Adjust Frequency
    rf95d.setFrequency(RF_FREQUENCY);
  
    //Adjust Power to 23 dBm
    rf95d.setTxPower(23, false);

    // Setup BandWidth, option: 7800,10400,15600,20800,31200,41700,62500,125000,250000,500000
    //Lower BandWidth for longer distance.
    rf95d.setSignalBandwidth(125000);
  
    // Setup Spreading Factor (6 ~ 12)
    rf95d.setSpreadingFactor(7);
    
    // Setup Coding Rate:5(4/5),6(4/6),7(4/7),8(4/8) 
    rf95d.setCodingRate4(5);
  
    // This is our Node ID
    rf95m.setThisAddress(RF_GATEWAY_ID);
    rf95m.setHeaderFrom(RF_GATEWAY_ID);
    //rf95m.setRetries(7); 
    }
}

uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
uint8_t tx_trg[] = "S";
uint8_t tlen = sizeof(tx_trg);

void loop(void) {
  int functionByte = 0;
  if (Serial.available() == 1)
    functionByte = Serial.read();
  
  if (functionByte == 83) { // 83 == S; if Arduino receives 'S' from RPi, broadcast 'S' to nodes
    rf95m.sendto(tx_trg, tlen, RH_BROADCAST_ADDRESS);
    rf95m.waitPacketSent();
    }
  
  if (rf95m.available()) {
    uint8_t len = sizeof(buf);
    uint8_t from;
    int8_t rssi = rf95d.lastRssi();
    if (rf95m.recvfromAck(buf, &len, &from)) { //If trigger is received from server, send recent data or accumulated data
      memcpy(&nodeData, buf, sizeof(nodeData));
      Serial.print("#"); Serial.print(from, DEC); Serial.print(",");
      Serial.print(rssi); Serial.print(",");
      Serial.print(nodeData.temp); Serial.print(",");
      Serial.print(nodeData.diox); Serial.print(",");
      Serial.print(nodeData.cdty); Serial.print("*");
    }
  }
}
