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
#define BEACON_META_LEN 26
#define MAX_MSG_LEN 161 // (RH_RF95_MAX_MESSAGE_LEN - BEACON_META_LEN)
#define MAX_SERIAL_OUT_LEN 640
#define MAX_SERIAL_IN_LEN 161

// SET APP SETTINGS HERE
#define CSV_FORMAT //NICE_FORMAT (Readable format for terminal) or CSV_FORMAT (Can be parsed with Beacon's Android App) to toggle
#define BROADCAST_INTERVAL_DURATION 2000 // DURATION BETWEEN BROADCASTS
//#define BROADCAST_ONLY_WITH_FIX

// Set DEBUG SETTINGS HERE
//#define MAX_DEBUG_LEN 640
//#define DEBUG_FUNCTION
//#define DEBUG_BEACON
//#define DEBUG_BUFFER
//#define DEBUG_NMEA
//#define DEBUG_SERIAL

// 26 metadata bytes + MAX_MSG_LEN bytes
struct BeaconData {     //stores the sensor values in a struct for easier sending and receiving via LoRa
  uint8_t hour, minute, seconds, year, month, day, fixq; // 1 byte each = 7 bytes
  char nsd, ewd; // 1 byte each = 2 bytes
  float latitude, longitude, altitude, hdop; // 4 bytes each = 16 bytes
  boolean fix; // 1 byte
  char msg[MAX_MSG_LEN]; // MAX_MSG_LEN bytes
};

// LOOP
uint32_t last_broadcast_time;

// DATA BUFFERS
BeaconData beaconData; // Storage for beacon data
char serial_out_buf[MAX_SERIAL_OUT_LEN]; // Buffer for formatting strings for serial output.
char serial_in_buf[MAX_SERIAL_IN_LEN]; // Buffer for serial input.
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

  #ifdef DEBUG_FUNCTION
  debug_log("Function", "lora_set_parameters");
  #endif
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
  
  #ifdef DEBUG_FUNCTION
  debug_log("Function", "Lora Setup");
  #endif
  
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
  
  #ifdef DEBUG_FUNCTION
  debug_log("Function", "Lora Init");
  #endif
  
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
  #ifdef DEBUG_FUNCTION
  debug_log("Function", "Broadcast Time Init: " + String(last_broadcast_time));
  #endif
}

void broadcast_time_stamp() {
  last_broadcast_time = millis(); // reset the last_broadcast_time
  #ifdef DEBUG_FUNCTION
  debug_log("Function", "Broadcast Time Stamp: " + String(last_broadcast_time));
  #endif
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
  #ifdef DEBUG_BUF
  debug_log("Function", "Clear Byte Array Buffer");
  #endif
  for (int i = 0; i < sizeof(buf); i++) { 
    buf[i] = 0;
  }
}

void clear_serial_in_buf(char* buf) {
  // Clear buffer content by filling it with NULL
  #ifdef DEBUG_BUF
  debug_log("Function", "Clear Char Array Buffer");
  #endif
  for (int i = 0; i < sizeof(buf); i++) { 
    buf[i] = 0;
  } 
}

// Returns false if we failed to parse a NMEA sentence from GPS
bool gps_parse_new_data() {
  
  #ifdef DEBUG_FUNCTION
  debug_log("Function", "GPS Parse New Data");
  #endif
  
  // read data from the GPS in the 'main loop'
  char c = GPS.read();
  
  // if you want to debug, this is a good time to do it!
  #ifdef DEBUG_NMEA
  if (c) debug_log("GPSECHO", String(c));
  #endif
  
  // if a sentence is received, we can check the checksum, parse it...
  if (GPS.newNMEAreceived()) {
    // a tricky thing here is if we print the NMEA sentence, or data
    // we end up not listening and catching other sentences!
    // so be very wary if using OUTPUT_ALLDATA and trytng to print out data
    char* nmea = GPS.lastNMEA();
    #ifdef DEBUG_NMEA
    debug_log("NMEA", nmea); // this also sets the newNMEAreceived() flag to false
    #endif
    if (!GPS.parse(nmea)) // this also sets the newNMEAreceived() flag to false
      return false; // we can fail to parse a sentence in which case we should just wait for another
  }
  return true;
}

// BEACON DATA HELPER FUNCTIONS

// Populate beacon data struct with GPS values
void gps_update_beacon_data(BeaconData* data) {
  #ifdef DEBUG_BEACON
  debug_log("Function", "GPS Update Beacon Data");
  #endif
  
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

  memcpy(data->msg, serial_in_buf, MAX_MSG_LEN);
}

void gps_clear_beacon_data(BeaconData* data) {
  #ifdef DEBUG_BEACON
  debug_log("Function", "GPS Clear Beacon Data");
  #endif
  
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

  // MSG BUF
  clear_serial_in_buf(data->msg);
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
  #ifdef DEBUG_SERIAL
  debug_log("Serial Read Message", "'" + String(buf) + "'");
  #endif
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
  sprintf(buf, "%d,%d,%d,%d,%d,%d,%f,%f,%d,%s",
    data->year, data->month, data->day, data->hour, data->minute, data->seconds, // 18 chars
    data->latitude, data->longitude, data->fix, // dddmm.mm,dddmm.mm,fff.ffff, 27
    data->fix ? data->msg : "NO GPS LOCK"); // 161
  Serial.println(buf);
  #endif

  #ifdef DEBUG_FUNCTION
  debug_log("Serial Print Beacon Data" , String(buf) );
  #endif
}

// TX HELPER FUNCTIONS

// Send beacon data over LoRa
void send_beacon_data(BeaconData* data) {
  //clear_buf(tx_buf); // clear buffer before populating with data.
  //memcpy(tx_buf, data, sizeof(*data)); // populate buffer for tx with beacon data
  lora_send((uint8_t*)data, sizeof(*data), rf_destination);
}

// Send byte array payload over LoRa
void lora_send(uint8_t* buf, uint8_t len, uint8_t address) {
  debug_log("LoRa Send" , String(len) + " bytes to node #" + String(address) );
  RF_MESSAGING.sendtoWait(buf, len, address);// send buffered data to aggregator
  RF_MESSAGING.waitPacketSent(); // wait until properly sent
}

// Broadcast generated data payload over LoRa
void broadcast_data() {

  // Populating beacon data
  gps_update_beacon_data(&beaconData); 
  
  // For Android App or Debug
  serial_print_beacon_data(serial_out_buf, &beaconData); // print beacon data to serial.

  #ifdef DEBUG_BEACON
  debug_log("GPS FIX" , beaconData.fix ? "true" : "false" );
  #endif 

  #ifdef BROADCAST_ONLY_WITH_FIX
  // If no fix, don't do anything.
  if (!beaconData.fix) 
    return;
  #endif

  // Actually send it.
  send_beacon_data(&beaconData);
}

// BEACON USE CASE

void beacon_setup() {

  // Set the hardware serial port to 9600, baudrate of the GPS receiver
  Serial.begin(SERIAL_BAUDRATE);
  
  // Initialize GPS
  gps_init();
  
  // Initialize Timer
  broadcast_time_init();

  // Clear Beacon Data
  gps_clear_beacon_data(&beaconData);
  
  // Initialize LoRa
  if (!lora_init())
    return;

  // TODO: Put here functions that should not be done until lora is initialized.
}

void update_input_stream() {
  
  // NOTE: GPS must be parsed and logged onto the GPS struct every frame.
  // If cannot parse new data, don't do anything.
  if(!gps_parse_new_data()) {
    // DO SOMETHING
  }

  // Read from serial
  serial_read_message(serial_in_buf); // populate beacon data struct with message from serial
}

void beacon_loop() {

  // Keep reading from input streams opportunistically.
  update_input_stream();
  
  // If waiting for broadcast interval, don't do anything.
  if(is_waiting_broadcast()) 
    return;

  // Process and broadcast data.
  broadcast_data();
  broadcast_time_stamp();
}

// SIMPLE USE CASE

uint8_t simple_index = 0;
char simple_buf[MAX_MSG_LEN] ;

void simple_broadcast_data(uint8_t* buf) {
  lora_send(buf, sizeof(buf), rf_destination);
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

// ARDUINO LIFE CYCLE

void setup() {
  beacon_setup();
}

void loop() {
  beacon_loop(); 
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
