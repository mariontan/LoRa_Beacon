#include <SPI.h>            //Arduino serial interface with the LoRa module and SD logger
#include <RHReliableDatagram.h>     //Manager library used for addressed, unreliable messages
#include <RH_RF95.h>        //Library used to send messages using LoRa
#include <Adafruit_GPS.h>   //Adafruit GPS Library for better parsing of data
#include <string.h>
#include <stdint.h>

// SET LORA NODE IDs HERE
#define RF_AGGREGATOR_ID 0 // Aggregator node's ID as receiver node ID.
#define RF_BEACON_ID    3 // ID for this node.

// SET DEFAULT LORA SETTINGS HERE
#define RF_FREQUENCY  433.00
#define RF_TX_POWER 23
#define RF_USE_RFO false
#define RF_BANDWIDTH 125000
#define RF_SPREAD_FACTOR 7
#define RF_CODING_RATE 5
#define RF_RETRIES 0

#define RF_SPREAD_FACTOR_7 7
#define RF_SPREAD_FACTOR_8 8
#define RF_SPREAD_FACTOR_10 10
#define RF_SPREAD_FACTOR_12 12

#define RF_BANDWIDTH_125KHZ 125000
#define RF_BANDWIDTH_62_5KHZ 62500

// SET DEFAULT SERIAL SETTINGS HERE
#define SERIAL_BAUDRATE 9600
#define GPS_BAUDRATE 9600
#define GPSSerial Serial1

// SET DEFAULT MESSAGE SETTINGS HERE
#define MAX_MSG_LEN 161
#define MAX_SERIAL_LEN 640
#define MAX_DEBUG_LEN 640

// SET APP SETTINGS HERE
#define GPSECHO false // Set GPSECHO to 'false' to turn off echoing the GPS data to the Serial console, 'true' if you want to debug and listen to the raw GPS sentences
#define CSV_FORMAT //NICE_FORMAT (Readable format for terminal) or CSV_FORMAT (Can be parsed with Beacon's Android App) to toggle
#define BROADCAST_INTERVAL_DURATION 2000 // DURATION BETWEEN BROADCASTS

struct BeaconData {     //stores the sensor values in a struct for easier sending and receiving via LoRa
  uint8_t hour, minute, seconds, year, month, day, fixq;
  char nsd, ewd;
  float latitude, longitude, altitude, hdop;
  boolean fix;
  char msg[MAX_MSG_LEN];
};

// LOOP
uint32_t last_broadcast_time;

// DATA BUFFERS
BeaconData beaconData; // Storage for beacon data
char serial_buf[MAX_SERIAL_LEN]; // Buffer for formatting strings.
uint8_t tx_buf[RH_RF95_MAX_MESSAGE_LEN]; // LoRa Byte Array payload buffer
#ifdef MAX_DEBUG_LEN
char debug_buf[MAX_DEBUG_LEN]; // Buffer for debugger
#endif

// LORA-GPS SETUP
RH_RF95 RF_DRIVER(8, 3);                          //Singleton instance of the radio driver
RHReliableDatagram RF_MESSAGING(RF_DRIVER, RF_BEACON_ID);  //This class manages message delivery and reception
Adafruit_GPS GPS(&GPSSerial);                 // Connect to the GPS on the hardware port
uint8_t rf_destination = RF_AGGREGATOR_ID;

// HELPER
void debug_log(String tag, String log_buf) {
  #ifdef MAX_DEBUG_LEN
  String buf = tag + ": " + log_buf;
  buf.toCharArray(debug_buf, MAX_DEBUG_LEN);
  Serial.println(debug_buf);
  #endif
}

// GPS INITIALIZATION
void gps_init(uint baudRate = GPS_BAUDRATE) {
  GPSSerial.begin(baudRate);
  
  // uncomment this line to turn on RMC (recommended minimum) and GGA (fix data) including altitude
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  
  // Set the update rate
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ); // 1 Hz update rate
}

// LORA INITIALIZATION

// Set RF95 Parameters
void lora_set_parameters(float frequency = RF_FREQUENCY, int8_t txPower = RF_TX_POWER, bool useRFO = RF_USE_RFO, long bandwidth = RF_BANDWIDTH, int8_t spreadFactor = RF_SPREAD_FACTOR, int8_t codingRate = RF_CODING_RATE) {

  debug_log("Function", "lora_set_parameters");
  
  //Adjust Frequency
  RF_DRIVER.setFrequency(frequency);
  debug_log("Frequency", String(frequency) );

  //Adjust Power to 23 dBm
  RF_DRIVER.setTxPower(txPower, useRFO);
  debug_log("TX power", String(txPower) );
  debug_log("Use RFO ", String(useRFO) );

  // Setup BandWidth, option: 7800,10400,15600,20800,31200,41700,62500,125000,250000,500000
  //Lower BandWidth for longer distance.
  RF_DRIVER.setSignalBandwidth(bandwidth);
  debug_log("Signal Bandwidth", String(bandwidth) );
  
  // Setup Spreading Factor (6 ~ 12)
  RF_DRIVER.setSpreadingFactor(spreadFactor);
  debug_log("Spread Factor", String(spreadFactor) );

  // Setup Coding Rate:5(4/5),6(4/6),7(4/7),8(4/8)
  RF_DRIVER.setCodingRate4(codingRate);
  debug_log("Coding Rate", String(codingRate) );
}

// Set this node's ID
void lora_set_source(uint8_t nodeID) {
  debug_log("Lora Set Source",String(nodeID));
  // This is our Node ID
  RF_MESSAGING.setThisAddress(nodeID);
  RF_MESSAGING.setHeaderFrom(nodeID);
}

// Set destination node's ID
void lora_set_destination(uint8_t nodeID, uint8_t retries = RF_RETRIES) {
  debug_log("Lora Set Destination",String(nodeID) + " <- " + String(retries) + " retries");
  // Where we're sending packet
  RF_MESSAGING.setHeaderTo(nodeID);
  if(retries > 0)
    RF_MESSAGING.setRetries(retries);
  rf_destination = nodeID;
}

// Set up driver and messaging service.
void lora_setup() {
  debug_log("Function", "Lora Setup");
  lora_set_parameters();
  //lora_set_parameters(RF_FREQUENCY,RF_TX_POWER,RF_USE_RFO, RF_BANDWIDTH_125KHZ, RF_SPREAD_FACTOR_7);
  //lora_set_parameters(RF_FREQUENCY,RF_TX_POWER,RF_USE_RFO, RF_BANDWIDTH_125KHZ, RF_SPREAD_FACTOR_8);
  //lora_set_parameters(RF_FREQUENCY,RF_TX_POWER,RF_USE_RFO, RF_BANDWIDTH_125KHZ, RF_SPREAD_FACTOR_10);
  //lora_set_parameters(RF_FREQUENCY,RF_TX_POWER,RF_USE_RFO, RF_BANDWIDTH_62_5KHZ, RF_SPREAD_FACTOR_10);
  //lora_set_parameters(RF_FREQUENCY,RF_TX_POWER,RF_USE_RFO, RF_BANDWIDTH_125KHZ, RF_SPREAD_FACTOR_12);
  //lora_set_parameters(RF_FREQUENCY,RF_TX_POWER,RF_USE_RFO, RF_BANDWIDTH_62_5KHZ, RF_SPREAD_FACTOR_12);
  lora_set_source(RF_BEACON_ID);
  lora_set_destination(RF_AGGREGATOR_ID);
}

// Initialize LoRa components.
bool lora_init() {
  debug_log("Function", "Lora Init");
  if (!RF_MESSAGING.init()){
    Serial.println("init failed");
    return false;
  }
  
  lora_setup();
  return true;
}

// TIME STAMP HELPER FUNCTIONS
void broadcast_time_init() {
  last_broadcast_time = millis() - BROADCAST_INTERVAL_DURATION;
  debug_log("Function", "Broadcast Time Init: " + String(last_broadcast_time));
}

void broadcast_time_stamp() {
  last_broadcast_time = millis(); // reset the last_broadcast_time
  debug_log("Function", "Broadcast Time Stamp: " + String(last_broadcast_time));
}

uint32_t get_duration(uint32_t timer_end, uint32_t timer_start) {
  // If the current time is greater than the last broadcast time, it still hasn't looped around. Get the duration normally.
  // If the last broadcast time is greater than the current time, current time has looped around. Get the looped around duration.
  return (timer_start <= timer_end) ? timer_end - timer_start : timer_end + (UINT32_MAX - timer_start);
}

// This service waits approximately every 30 seconds or so in between broadcasts.
// Returns true if it's still waiting before it can broadcast again.
// Returns false if it's done waiting before it can broadcast again.
bool is_waiting_broadcast() {
  return (get_duration(millis(), last_broadcast_time) <= BROADCAST_INTERVAL_DURATION);
}

// BUFFER HELPER FUNCTIONS
void clear_buf(uint8_t* buf) {
  // Clear buffer content by filling it with NULL
  debug_log("Function", "Clear Byte Array Buffer");
  for (int i = 0; i < sizeof(buf); i++) { 
    buf[i] = 0;
  }
}

void clear_msg_buf(char* buf) {
  // Clear buffer content by filling it with NULL
  debug_log("Function", "Clear Char Array Buffer");
  for (int i = 0; i < sizeof(buf); i++) { 
    buf[i] = 0;
  } 
}

// Returns false if we failed to parse a NMEA sentence from GPS
bool gps_parse_new_data() {
  debug_log("Function", "GPS Parse New Data");
  // read data from the GPS in the 'main loop'
  char c = GPS.read();
  
  // if you want to debug, this is a good time to do it!
  if (c) debug_log("GPSECHO", String(c));
    
  // if a sentence is received, we can check the checksum, parse it...
  if (GPS.newNMEAreceived()) {
    // a tricky thing here is if we print the NMEA sentence, or data
    // we end up not listening and catching other sentences!
    // so be very wary if using OUTPUT_ALLDATA and trytng to print out data
    char* nmea = GPS.lastNMEA();
    debug_log("NMEA", nmea); // this also sets the newNMEAreceived() flag to false
    if (!GPS.parse(nmea)) // this also sets the newNMEAreceived() flag to false
      return false; // we can fail to parse a sentence in which case we should just wait for another
  }
  return true;
}

// BEACON DATA HELPER FUNCTIONS

// Populate beacon data struct with GPS values
void gps_update_beacon_data(BeaconData* data) {
  debug_log("Function", "GPS Update Beacon Data");
  
  // GPS FIX
  data->fix = GPS.fix;
  data->fixq = GPS.fixquality;

  // GPS DATE TIME
  data->hour = GPS.hour;
  data->minute = GPS.minute;
  data->seconds = GPS.seconds;
  data->day = GPS.day;
  data->month = GPS.month;
  data->year = GPS.year;

  // GPS POSITION
  data->latitude = !GPS.fix ? 0 : GPS.latitude;
  data->nsd = !GPS.fix ? 0 : GPS.lat;
  data->longitude = !GPS.fix ? 0 : GPS.longitude;
  data->ewd = !GPS.fix ? 0 : GPS.lon;
  data->altitude = !GPS.fix ? 0 : GPS.altitude;
  data->hdop = !GPS.fix ? 0 : GPS.HDOP;
}

void gps_clear_beacon_data(BeaconData* data) {
  debug_log("Function", "GPS Clear Beacon Data");
  
  // GPS FIX
  data->fix = false;
  data->fixq = 0;

  // GPS DATE TIME
  data->hour = 0;
  data->minute = 0;
  data->seconds = 0;
  data->day = 0;
  data->month = 0;
  data->year = 0;

  // GPS POSITION
  data->latitude = 0;
  data->nsd = 0;
  data->longitude = 0;
  data->ewd = 0;
  data->altitude = 0;
  data->hdop = 0;

  clear_msg_buf(data->msg);
}

// Populate message buffer from Serial
void serial_read_message(char* buf) {
  int bufSize = sizeof(buf);
  byte i = 0;
  
  // Don't read unless there you know there is data. Stop reading if one less than the size of the array.
  while (Serial.available() > 0 && i < bufSize-1) { 
    buf[i++] = Serial.read(); // Read a character // Store it. Increment where to write next
  }
  
  buf[i] = '\0'; // Null terminate the string
  debug_log("Serial Read Message", "'" + String(buf) + "'");
}

// Populate beacon data struct with GPS and Serial Message. Map it onto payload buffer for sending.
void populate_beacon_data(BeaconData* data) {
  debug_log("Function", "Populate Beacon Data");
  gps_update_beacon_data(data); // populate beacon data struct with GPS values
  serial_read_message(data->msg); // populate beacon data struct with message from serial
}

// Print beacon data to serial
void serial_print_beacon_data(char* buf, BeaconData* data) {
  
  #ifdef NICE_FORMAT
  sprintf(buf, 
    "\nTime: %2d:%2d:%2d.%d\nDate: %2d/%2d/20%2d\nFix: %d Quality:%d\nMessage: %s\nSending Beacon Signal...\nLocation: %f%s, %f%s\nAltitude: %f",
    data->hour, data->minute, data->seconds, GPS.milliseconds, 
    data->day, data->month, data->year, 
    (int)data->fix, (int)data->fixq, data->msg,
    data->latitude, data->nsd, data->longitude, data->ewd, data->altitude);
  Serial.println(buf);
  #endif
  
  #ifdef CSV_FORMAT // change to Single serial command, change to csv format for easy android parsing
  sprintf(buf, "%d,%d,%d,%d,%d,%d,%f,%f,%s,%s",
    data->year, data->month, data->day, data->hour, data->minute, data->seconds, // 18 chars
    data->latitude, data->longitude, data->fix ? "true" : "false", // dddmm.mm,dddmm.mm,fff.ffff, 27
    data->msg); // 161
  Serial.println(buf);
  #endif

  
  debug_log("Serial Print Beacon Data" , String(buf) );
}

// TX HELPER FUNCTIONS

// Send beacon data over LoRa
void send_beacon_data(BeaconData* data) {
  clear_buf(tx_buf); // clear buffer before populating with data.
  memcpy(tx_buf, data, sizeof(*data)); // populate buffer for tx with beacon data
  lora_send(tx_buf, sizeof(beaconData), rf_destination);
}

// Send byte array payload over LoRa
void lora_send(uint8_t* buf, uint8_t len, uint8_t address) {
  debug_log("LoRa Send" , String(len) + " bytes to node #" + String(address) );
  RF_MESSAGING.sendtoWait(buf, len, address);// send buffered data to aggregator
  RF_MESSAGING.waitPacketSent(); // wait until properly sent
}

// Broadcast generated data payload over LoRa
void broadcast_data() {

  // If cannot parse new data, don't do anything.
  if(!gps_parse_new_data()){
    // Populating beacon data with empty values
    gps_clear_beacon_data(&beaconData);
  }
  else {
    // Populating beacon data with values from GPS and Serial
    populate_beacon_data(&beaconData); // populate beacon data struct.
  }
  
  // For Android App or Debug
  serial_print_beacon_data(serial_buf, &beaconData); // print beacon data to serial.

  debug_log("GPS FIX" , beaconData.fix ? "true" : "false" );
  
  // If no fix, don't do anything.
  if (!beaconData.fix) 
    return;

  // Actually send it.
  send_beacon_data(&beaconData);
}

// ARDUINO LIFE CYCLE

void new_setup() {

  // Set the hardware serial port to 9600, baudrate of the GPS receiver
  Serial.begin(SERIAL_BAUDRATE);
  
  // Initialize GPS
  // gps_init();


  GPSSerial.begin(9600);
  // uncomment this line to turn on RMC (recommended minimum) and GGA (fix data) including altitude
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  // Set the update rate
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ); // 1 Hz update rate
  
  // Initialize Timer
  broadcast_time_init();
  
  // Initialize LoRa
  if (!lora_init())
    return;

  // TODO: Put here functions that should not be done until lora is initialized.
}

void new_loop() {

  // If cannot broadcast, wait.
  if(is_waiting_broadcast()) 
    return;

  // process and broadcast data
  broadcast_data();
  broadcast_time_stamp();
}


void old_setup() {
  Serial.begin(9600);              // Set the hardware serial port to 9600, baudrate of the GPS receiver
  GPSSerial.begin(9600);
  // uncomment this line to turn on RMC (recommended minimum) and GGA (fix data) including altitude
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  // Set the update rate
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ); // 1 Hz update rate

  /*--Initializing LoRa module--*/
  if (RF_MESSAGING.init()) {
    //Adjust Frequency
    RF_DRIVER.setFrequency(RF_FREQUENCY);

    //Adjust Power to 23 dBm
    RF_DRIVER.setTxPower(23, false);

    // Setup BandWidth, option: 7800,10400,15600,20800,31200,41700,62500,125000,250000,500000
    //Lower BandWidth for longer distance.
    RF_DRIVER.setSignalBandwidth(125000);

    // Setup Spreading Factor (6 ~ 12)
    RF_DRIVER.setSpreadingFactor(7);

    // Setup Coding Rate:5(4/5),6(4/6),7(4/7),8(4/8)
    RF_DRIVER.setCodingRate4(5);

    // This is our Node ID
    RF_MESSAGING.setThisAddress(RF_BEACON_ID);
    RF_MESSAGING.setHeaderFrom(RF_BEACON_ID);

    // Where we're sending packet
    RF_MESSAGING.setHeaderTo(RF_AGGREGATOR_ID);
    //RF_MESSAGING.setRetries(7);
  }
  for (int i = 0; i < sizeof(tx_buf); i++) { // Initialize buffer for transmitted data by filling with NULL
    tx_buf[i] = 0;
  }
}

void old_loop() {
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
  if (last_broadcast_time > millis()) last_broadcast_time = millis();
  // approximately every 30 seconds or so, print out the current stats
  if (millis() - last_broadcast_time > 2000) {
    last_broadcast_time = millis(); // reset the timer

    #ifdef NICE_FORMAT
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
    #endif
    byte index1 = 0;//reset index back to 0
    
    while (Serial.available() > 0) // Don't read unless
                                   // there you know there is data
    {
        if(index1 < 160) // One less than the size of the array
        {
            beaconData.msg[index1] =Serial.read(); // Read a character // Store it
            index1++; // Increment where to write next
            beaconData.msg[index1] = '\0'; // Null terminate the string
        }
        
    }
    #ifdef NICE_FORMAT
    Serial.print("message:");
    Serial.println(beaconData.msg);
    #endif
    if (GPS.fix) {
      #ifdef NICE_FORMAT
      Serial.println("Sending Beacon Signal...");
      Serial.print("Location: ");
      Serial.print(GPS.latitude, 4); Serial.print(GPS.lat);
      Serial.print(", ");
      Serial.print(GPS.longitude, 4); Serial.println(GPS.lon);
      Serial.print("Altitude: "); Serial.println(GPS.altitude);
      Serial.println();
      #endif
      beaconData.fix = GPS.fix;
      beaconData.fixq = GPS.fixquality;
      beaconData.hdop = GPS.HDOP;
      beaconData.hour = GPS.hour;
      beaconData.minute = GPS.minute;
      beaconData.seconds = GPS.seconds;
      beaconData.day = GPS.day;
      beaconData.month = GPS.month;
      beaconData.year = GPS.year;
      beaconData.latitude = GPS.latitude;
      beaconData.nsd = GPS.lat;
      beaconData.longitude = GPS.longitude;
      beaconData.ewd = GPS.lon;
      beaconData.altitude = GPS.altitude;
      memcpy(tx_buf, &beaconData, sizeof(beaconData));
      #ifdef CSV_FORMAT //change to Single serial command, change to csv format for easy android parsing
      String gpsYear = String(beaconData.year);
      String gpsMon = String(beaconData.month);
      String gpsDay = String(beaconData.day);
      String gpsHour = String(beaconData.hour);//store GPS.hour to beaaconData.hr to change cast properly uint8_t test = GPS.hour yields a value of 0 even if GPS.hour is not 0
      String gpsMin = String(beaconData.minute);
      String gpsSec = String(beaconData.seconds);
      String gpsLat = String(beaconData.latitude);
      String gpsLon = String(beaconData.longitude);
      String gpsFix = String(beaconData.fix);
      Serial.println(gpsYear+","+gpsMon+","+gpsDay+","+gpsHour+","+gpsMin+","+gpsSec+","+gpsLat+","+gpsLon+","+gpsFix+","+beaconData.msg);
      #endif
      RF_MESSAGING.sendtoWait(tx_buf, sizeof(beaconData), RF_AGGREGATOR_ID);
      RF_MESSAGING.waitPacketSent();
      
    }
    else  Serial.println("0,0,0,0,0,0,0,0,0,no GPS LOCK");
          
  }
  for (int i = 0; i < sizeof(tx_buf); i++) {
    tx_buf[i] = 0;
  }
}

void fix_loop() {
 // uint8_t tx_buf[RH_RF95_MAX_MESSAGE_LEN]; // Alloted 251 bytes for tx_buf.
  // read data from the GPS in the 'main loop'
  /*
  char c = GPS.read();
  // if a sentence is received, we can check the checksum, parse it...
  if (GPS.newNMEAreceived()) {
    // a tricky thing here is if we print the NMEA sentence, or data
    // we end up not listening and catching other sentences!
    // so be very wary if using OUTPUT_ALLDATA and trytng to print out data
    char* nmea = GPS.lastNMEA();
    debug_log("NMEA", nmea); // this also sets the newNMEAreceived() flag to false
    if (!GPS.parse(nmea)) // this also sets the newNMEAreceived() flag to false
      return; // we can fail to parse a sentence in which case we should just wait for another
  }
  //*/
  if(!gps_parse_new_data())
    return;
  // if millis() or timer wraps around, we'll just reset it
  if (last_broadcast_time > millis()) last_broadcast_time = millis();
  // approximately every 30 seconds or so, print out the current stats
  if (millis() - last_broadcast_time > 2000) {
    last_broadcast_time = millis(); // reset the timer

    serial_read_message(beaconData.msg); // populate beacon data struct with message from serial
    if (GPS.fix) {
      populate_beacon_data(&beaconData); // populate beacon data struct.
      send_beacon_data(&beaconData);;
      
    }
    
    serial_print_beacon_data(serial_buf, &beaconData); // print beacon data to serial.
  }
  clear_buf(tx_buf);
}

void simple_setup() {
  
  // Set the hardware serial port to 9600, baudrate of the GPS receiver
  Serial.begin(SERIAL_BAUDRATE);

  // Initialize Timer
  broadcast_time_init();
  
  // Initialize LoRa
  if (!lora_init())
    return;
}

void simple_broadcast_data(uint8_t* buf) {
  lora_send(buf, sizeof(buf), rf_destination);
}

uint8_t simple_index = 0;
char simple_buf[MAX_MSG_LEN] ;

void simple_loop() {
  
  // If cannot broadcast, wait.
  if(is_waiting_broadcast()) 
    return;

  // process and broadcast data
  sprintf(simple_buf, "%d", simple_index++);
  memcpy(tx_buf, simple_buf, sizeof(simple_buf)); 
  debug_log("Sending", simple_buf);
  simple_broadcast_data(tx_buf);
  broadcast_time_stamp();
}

void setup() {
  simple_setup();
}

void loop() {
  simple_loop(); 
}

/***getting the data type of variable***/
/*// Generic catch-all implementation. 
template <typename T_ty> struct TypeInfo { static const char * name; };
template <typename T_ty> const char * TypeInfo<T_ty>::name = "unknown";

// Handay macro to make querying stuff easier.
#define TYPE_NAME(var) TypeInfo< typeof(var) >::name

// Handay macro to make defining stuff easier.
#define MAKE_TYPE_INFO(type)  template <> const char * TypeInfo<type>::name = #type;

// Type-specific implementations.
MAKE_TYPE_INFO( int )
MAKE_TYPE_INFO( float )
MAKE_TYPE_INFO( short )
MAKE_TYPE_INFO( uint8_t )*/
