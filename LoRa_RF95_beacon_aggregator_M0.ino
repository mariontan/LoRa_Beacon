// rf95_reliable_datagram_server.pde
// -*- mode: C++ -*-
// Example sketch showing how to create a simple addressed, reliable messaging server
// with the RHReliableDatagram class, using the RH_RF95 driver to control a RF95 radio.
// It is designed to work with the other example rf95_reliable_datagram_client
// Tested with Anarduino MiniWirelessLoRa, Rocket Scream Mini Ultra Pro with the RFM95W 

#include <RHReliableDatagram.h>
#include <RH_RF95.h>
#include <SPI.h>

#define RF_FREQUENCY      433.00 
#define RF_AGGREGATOR_ID  0

RH_RF95 rf95d(8,3);                                //Singleton instance of the radio driver
RHReliableDatagram rf95m(rf95d, RF_AGGREGATOR_ID);  //This class manages message delivery and reception

struct dataStruct {     //stores the sensor values in a struct for easier sending and receiving via LoRa
  uint8_t hr, mnt, sec, yr, mth, dy, fixq;
  char nsd, ewd;
  float lattd, longtd, alttd, hdop;
  boolean fix;
} beaconData;

char utctime[8], utcdate[8], nsd, ewd;

// Need this on Arduino Zero with SerialUSB port (eg RocketScream Mini Ultra Pro)
//#define Serial SerialUSB

void setup() 
{  
  Serial.begin(9600);
  while (!Serial) ; // Wait for serial port to be available

  if (!rf95m.init())
    Serial.println("init failed");
    
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
    rf95m.setThisAddress(RF_AGGREGATOR_ID);
    rf95m.setHeaderFrom(RF_AGGREGATOR_ID);

}

// Dont put this on the stack:
uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];

void loop()
{
  if (rf95m.available())
  {
    // Wait for a message addressed to us from the client
    uint8_t len = sizeof(buf);
    uint8_t from, id;
    int8_t rssi = rf95d.lastRssi();
    if (rf95m.recvfromAck(buf, &len, &from, NULL, &id))
    {
      memcpy(&beaconData, buf, sizeof(beaconData));
      if (beaconData.fix && beaconData.fixq > 0){
        // Reconstruct UTC date and time and write all beacon data to serial
        sprintf(utcdate, "%02d%02d%02d", beaconData.dy, beaconData.mth, beaconData.yr);
        sprintf(utctime, "%02d%02d%02d", beaconData.hr, beaconData.mnt, beaconData.sec);
        nsd = (char)beaconData.nsd;
        ewd = (char)beaconData.ewd;
        Serial.print("#");
        Serial.print((char*)utcdate); Serial.print(",");
        Serial.print((char*)utctime); Serial.print(";");
        Serial.print(from); Serial.print(";");
        Serial.print(rssi); Serial.print(";");      
        Serial.print(beaconData.lattd, 4); Serial.print(";");
        Serial.print((char)nsd); Serial.print(";");
        Serial.print(beaconData.longtd, 4); Serial.print(";");
        Serial.print((char)ewd); Serial.print(";");
        Serial.print(beaconData.alttd); Serial.print(";");
        Serial.print(beaconData.hdop);
        Serial.println("*");
      }
    }
  }
}
