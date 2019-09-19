#include <SPI.h>            //Arduino serial interface with the LoRa module and SD logger
#include <RHReliableDatagram.h>     //Manager library used for addressed, unreliable messages
#include <RH_RF95.h>        //Library used to send messages using LoRa
#include <Adafruit_GPS.h>   //Adafruit GPS Library for better parsing of data
#include <string.h>

#define RF_FREQUENCY  433.00
#define RF_AGGREGATOR_ID 0
#define RF_NODE_ID    5
#define GPSSerial Serial1
#define MAX_MSG_LEN 161

RH_RF95 rf95d(8, 3);                          //Singleton instance of the radio driver
RHReliableDatagram rf95m(rf95d, RF_NODE_ID);  //This class manages message delivery and reception
Adafruit_GPS GPS(&GPSSerial);                 // Connect to the GPS on the hardware port

struct dataStruct {     //stores the sensor values in a struct for easier sending and receiving via LoRa
  uint8_t hr, mnt, sec, yr, mth, dy, fixq;
  char nsd, ewd;
  float lattd, longtd, alttd, hdop;
  boolean fix;
  char msg[MAX_MSG_LEN];
} beaconData;

// Set GPSECHO to 'false' to turn off echoing the GPS data to the Serial console
// Set to 'true' if you want to debug and listen to the raw GPS sentences
#define GPSECHO false

#define CSVString //NICEoutput or CSVString to toggle 

uint8_t tx_buf[RH_RF95_MAX_MESSAGE_LEN];

uint32_t timer = millis();

char inChar=-1; // Where to store the character read
byte index1 = 0; // Index into array; where to store the character

String gpsYear,gpsMon,gpsDay,gpsHour,gpsMin,gpsSec,gpsLat,gpsLon,gpsFix = "";


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
  uint8_t tx_buf[1] = {10}; // Alloted 1 byte for tx_buf.
  // if millis() or timer wraps around, we'll just reset it
  if (timer > millis()) timer = millis();
  // approximately every 30 seconds or so, print out the current stats
  if (millis() - timer > 1000) {
    timer = millis(); // reset the timer

      rf95m.sendtoWait(tx_buf, sizeof(tx_buf), RF_AGGREGATOR_ID);
      rf95m.waitPacketSent();          
  }
}

/***getting the data type of variable***/
/*// Generic catch-all implementation. 
template <typename T_ty> struct TypeInfo { static const char * name; };
template <typename T_ty> const char * TypeInfo<T_ty>::name = "unknown";

// Handy macro to make querying stuff easier.
#define TYPE_NAME(var) TypeInfo< typeof(var) >::name

// Handy macro to make defining stuff easier.
#define MAKE_TYPE_INFO(type)  template <> const char * TypeInfo<type>::name = #type;

// Type-specific implementations.
MAKE_TYPE_INFO( int )
MAKE_TYPE_INFO( float )
MAKE_TYPE_INFO( short )
MAKE_TYPE_INFO( uint8_t )*/
