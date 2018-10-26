#include <SPI.h>            //Arduino serial interface with the LoRa module and SD logger
#include <RHReliableDatagram.h>     //Manager library used for addressed, unreliable messages
#include <RH_RF95.h>        //Library used to send messages using LoRa
#include <Adafruit_GPS.h>   //Adafruit GPS Library for better parsing of data
#include <string.h>

#define RF_FREQUENCY  433.00
#define RF_AGGREGATOR_ID 0
#define RF_NODE_ID    4
#define GPSSerial Serial1

RH_RF95 rf95d(8, 3);                          //Singleton instance of the radio driver
RHReliableDatagram rf95m(rf95d, RF_NODE_ID);  //This class manages message delivery and reception
Adafruit_GPS GPS(&GPSSerial);                 // Connect to the GPS on the hardware port

struct dataStruct {     //stores the sensor values in a struct for easier sending and receiving via LoRa
  uint8_t hr, mnt, sec, yr, mth, dy, fixq;
  char nsd, ewd;
  float lattd, longtd, alttd, hdop;
  boolean fix;
} beaconData;

// Set GPSECHO to 'false' to turn off echoing the GPS data to the Serial console
// Set to 'true' if you want to debug and listen to the raw GPS sentences
#define GPSECHO false

uint8_t tx_buf[RH_RF95_MAX_MESSAGE_LEN];

uint32_t timer = millis();

void setup() {
  Serial.begin(9600);              // Set the hardware serial port to 9600, baudrate of the GPS receiver
  GPSSerial.begin(9600);
  // uncomment this line to turn on RMC (recommended minimum) and GGA (fix data) including altitude
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  // Set the update rate
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ); // 1 Hz update rate

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
    rf95m.setThisAddress(RF_NODE_ID);
    rf95m.setHeaderFrom(RF_NODE_ID);

    // Where we're sending packet
    rf95m.setHeaderTo(RF_AGGREGATOR_ID);
    //rf95m.setRetries(7);
  }
  for (int i = 0; i < sizeof(tx_buf); i++) { // Initialize buffer for transmitted data by filling with NULL
    tx_buf[i] = 0;
  }
}

void loop() {
  uint8_t tx_buf[RH_RF95_MAX_MESSAGE_LEN]; // Alloted 251 bytes for tx_buf.
  // read data from the GPS in the 'main loop'
  char c = GPS.read();
  // if you want to debug, this is a good time to do it!
  if (GPSECHO)
    if (c) Serial.print(c);
  // if a sentence is received, we can check the checksum, parse it...
  if (GPS.newNMEAreceived()) {
    // a tricky thing here is if we print the NMEA sentence, or data
    // we end up not listening and catching other sentences!
    // so be very wary if using OUTPUT_ALLDATA and trytng to print out data
    //    Serial.println(GPS.lastNMEA()); // this also sets the newNMEAreceived() flag to false
    if (!GPS.parse(GPS.lastNMEA())) // this also sets the newNMEAreceived() flag to false
      return; // we can fail to parse a sentence in which case we should just wait for another
  }
  // if millis() or timer wraps around, we'll just reset it
  if (timer > millis()) timer = millis();
  // approximately every 30 seconds or so, print out the current stats
  if (millis() - timer > 30000) {
    timer = millis(); // reset the timer
    Serial.print("\nTime: ");
    Serial.print(GPS.hour, DEC); Serial.print(':');
    Serial.print(GPS.minute, DEC); Serial.print(':');
    Serial.print(GPS.seconds, DEC); Serial.print('.');
    Serial.println(GPS.milliseconds);
    Serial.print("Date: ");
    Serial.print(GPS.day, DEC); Serial.print('/');
    Serial.print(GPS.month, DEC); Serial.print("/20");
    Serial.println(GPS.year, DEC);
    Serial.print("Fix: "); Serial.print((int)GPS.fix);
    Serial.print(" quality: "); Serial.println((int)GPS.fixquality);
    if (GPS.fix) {
      Serial.println("Sending Beacon Signal...");
      Serial.print("Location: ");
      Serial.print(GPS.latitude, 4); Serial.print(GPS.lat);
      Serial.print(", ");
      Serial.print(GPS.longitude, 4); Serial.println(GPS.lon);
      Serial.print("Altitude: "); Serial.println(GPS.altitude);
      Serial.println();
      beaconData.fix = GPS.fix;
      beaconData.fixq = GPS.fixquality;
      beaconData.hdop = GPS.HDOP;
      beaconData.hr = GPS.hour;
      beaconData.mnt = GPS.minute;
      beaconData.sec = GPS.seconds;
      beaconData.dy = GPS.day;
      beaconData.mth = GPS.month;
      beaconData.yr = GPS.year;
      beaconData.lattd = GPS.latitude;
      beaconData.nsd = GPS.lat;
      beaconData.longtd = GPS.longitude;
      beaconData.ewd = GPS.lon;
      beaconData.alttd = GPS.altitude;
      memcpy(tx_buf, &beaconData, sizeof(beaconData));
      rf95m.sendtoWait(tx_buf, sizeof(beaconData), RF_AGGREGATOR_ID);
      rf95m.waitPacketSent();
    }
    else Serial.println("No Fix. No Beacon Signal to Send.");
  }
  for (int i = 0; i < sizeof(tx_buf); i++) {
    tx_buf[i] = 0;
  }
}
