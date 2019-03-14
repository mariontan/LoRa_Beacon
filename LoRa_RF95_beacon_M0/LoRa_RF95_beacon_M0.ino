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
#define BEACON_META_LEN 24
#define MAX_MSG_LEN (RH_RF95_MAX_MESSAGE_LEN - BEACON_META_LEN) // 161
#define MAX_SERIAL_OUT_LEN 640
#define MAX_SERIAL_IN_LEN 161

// SET APP SETTINGS HERE
#define CSV_FORMAT //NICE_FORMAT (Readable format for terminal) or CSV_FORMAT (Can be parsed with Beacon's Android App) to toggle
#define BROADCAST_INTERVAL_MILLIS 2000 // DURATION BETWEEN BROADCASTS
//#define BROADCAST_ONLY_WITH_FIX

// Set DEBUG SETTINGS HERE
//#define MAX_DEBUG_LEN 640
//#define DEBUG_FUNCTION
//#define DEBUG_BEACON
//#define DEBUG_BUFFER
//#define DEBUG_NMEA
//#define DEBUG_SERIAL

// 24 metadata bytes + MAX_MSG_LEN bytes
struct BeaconData {     //stores the sensor values in a struct for easier sending and receiving via LoRa
  uint8_t hour, minute, seconds, year, month, day, fixq; // 1 byte each = 7 bytes
  float latitude, longitude, altitude, hdop; // 4 bytes each = 16 bytes
  boolean fix; // 1 byte
  char msg[MAX_MSG_LEN]; // MAX_MSG_LEN bytes
};

// LOOP
uint32_t last_broadcast_millis;

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

// TIME-DEPENDENT FUNCTIONS
void broadcast_time_init() {
  last_broadcast_millis = millis() - BROADCAST_INTERVAL_MILLIS;
  #ifdef DEBUG_FUNCTION
  debug_log("Function", "Broadcast Time Init: " + String(last_broadcast_millis));
  #endif
}

void broadcast_time_stamp() {
  last_broadcast_millis = millis(); // reset the last_broadcast_millis
  #ifdef DEBUG_FUNCTION
  debug_log("Function", "Broadcast Time Stamp: " + String(last_broadcast_millis));
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
bool is_waiting_broadcast(uint32_t interval = BROADCAST_INTERVAL_MILLIS) {
  return (get_duration(millis(), last_broadcast_millis) <= interval);
}

// broadcast: the function for broadcasting
// interval: time between broadcasts
void broadcast_loop(void (*broadcast)(), uint32_t interval = BROADCAST_INTERVAL_MILLIS) { 
  // If cannot broadcast, wait.
  if(is_waiting_broadcast()) 
    return;
  (*broadcast)();
  broadcast_time_stamp();
}

// BUFFER HELPER FUNCTIONS
template<typename T> void clear_buf(T* buf) {
  // Clear buffer content by filling it with NULL
  #ifdef DEBUG_BUF
  debug_log("Function", "Clear Array Buffer");
  #endif
  for (int i = 0; i < sizeof(buf); i++) { buf[i] = 0; }
}

// INPUT STREAM FUNCTIONS

// Populate message buffer from Serial
bool serial_read_message(char* buf, uint8_t len) {
  // If there's nothing to read, do nothing.
  if(Serial.available() <= 0) { return false; }
  
  // Don't read unless there you know there is data. Stop reading if one less than the size of the array.
  // Read a character. Store it. Increment where to write next
  byte i = 0;
  while (Serial.available() > 0 && i < len-1) { buf[i++] = Serial.read(); } 
  buf[i] = 0; // Null terminate the string
  
  #ifdef DEBUG_SERIAL
  debug_log("Serial Read Message", "'" + String(buf) + "'");
  #endif
  
  return true;
}

// Returns false if we failed to parse a NMEA sentence from GPS
bool gps_parse_new_data(char* buf = NULL) {
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
  if (!GPS.newNMEAreceived()) return false;
  // a tricky thing here is if we print the NMEA sentence, or data
  // we end up not listening and catching other sentences!
  // so be very wary if using OUTPUT_ALLDATA and trytng to print out data
  char* nmea = GPS.lastNMEA(); // this also sets the newNMEAreceived() flag to false
  #ifdef DEBUG_NMEA
  debug_log("NMEA", nmea); 
  #endif
  if (!GPS.parse(nmea)) return false; // we can fail to parse a sentence in which case we should just wait for another
  if(buf != NULL) memcpy(buf, nmea, sizeof(nmea));
  return true;
}

void beacon_interpret_serial(char* in, uint8_t len) {
  // If it's a message: do this
  memcpy(beaconData.msg, in, len < MAX_MSG_LEN ? len : MAX_MSG_LEN);
  // Else
  // DO SOME OTHER THING

  // Note: Can interpret commands from input stream aside from just messages.
}

void beacon_input_stream() {
  
  // Read from serial
  if(serial_read_message(serial_in_buf, MAX_SERIAL_IN_LEN)) { // populate beacon data struct with message from serial
    // DO SOMETHING
    beacon_interpret_serial(serial_in_buf, MAX_SERIAL_IN_LEN);
  } else {
    // DO SOME CORRECTION
  }
  
  // NOTE: GPS must be parsed and logged onto the GPS struct every frame.
  // If cannot parse new data, don't do anything.
  if(gps_parse_new_data()) {
    // DO SOMETHING
  } else {
    // DO SOME CORRECTION
  }
}

// BEACON DATA HELPER FUNCTIONS

// Populate beacon data struct with GPS values
void gps_update_beacon_data(BeaconData* data, Adafruit_GPS* gps_in) {
  #ifdef DEBUG_BEACON
  debug_log("Function", "GPS Update Beacon Data");
  #endif
  
  // GPS FIX
  data->fix = gps_in->fix;
  data->fixq = gps_in->fixquality;

  // GPS DATE TIME
  data->hour = gps_in->hour;
  data->minute = gps_in->minute;
  data->seconds = gps_in->seconds;
  data->day = gps_in->day;
  data->month = gps_in->month;
  data->year = gps_in->year;

  // GPS POSITION
  data->latitude = !gps_in->fix ? 0 : gps_in->latitudeDegrees;
  data->longitude = !gps_in->fix ? 0 : gps_in->longitudeDegrees;
  data->altitude = !gps_in->fix ? 0 : gps_in->altitude;
  data->hdop = !gps_in->fix ? 0 : gps_in->HDOP;
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
  data->longitude = 0;
  data->altitude = 0;
  data->hdop = 0;
}

// Print beacon data to serial
void serial_print_beacon_data(BeaconData* data, char* buf) {

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
template<typename T> void lora_send(T* buf, uint8_t len, uint8_t address) {
  debug_log("LoRa Send" , String(len) + " bytes to node #" + String(address) );
  RF_MESSAGING.sendtoWait((uint8_t*)buf, len, address);// send buffered data to aggregator
  RF_MESSAGING.waitPacketSent(); // wait until properly sent
}

// Broadcast generated data payload over LoRa
void broadcast_beacon_data() {

  // Populating beacon data
  gps_update_beacon_data(&beaconData, &GPS); 
  
  // For Android App or Debug
  serial_print_beacon_data(&beaconData, serial_out_buf); // print beacon data to serial.

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
  
  // Clear Beacon Data
  gps_clear_beacon_data(&beaconData);
  
  // Initialize LoRa
  if (!lora_init()) return;

  // TODO: Put here functions that should not be done until lora is initialized.
}

// SIMPLE USE CASE

uint8_t simple_index = 0;

void broadcast_test_data() {
  sprintf(serial_in_buf, "%d", simple_index++);
  #ifdef DEBUG_FUNCTION
  debug_log("Sending", serial_in_buf);
  #endif
  lora_send(serial_in_buf, sizeof(serial_in_buf), rf_destination);
}

void simple_setup() {
  Serial.begin(SERIAL_BAUDRATE); // Set the hardware serial port to 9600, baudrate of the GPS receiver
  if (!lora_init()) return; // Initialize LoRa
}

// MAIN LIFE CYCLE

void setup() {
  // Initialize Timer
  broadcast_time_init();
  beacon_setup();
  //simple_setup();
}

void loop() {
  beacon_input_stream(); 
  broadcast_loop(broadcast_beacon_data);
  //broadcast_loop(broadcast_test_data);
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
