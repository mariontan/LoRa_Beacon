#include <Adafruit_GPS.h>   //Adafruit GPS Library for better parsing of data
#define GPS_BAUDRATE 9600
#define GPSSerial Serial1

Adafruit_GPS GPS(&GPSSerial);                 // Connect to the GPS on the hardware port

// GPS INITIALIZATION
void gps_init(uint baudRate = GPS_BAUDRATE) {
  
  // Set the hardware serial port to the baudrate of the GPS receiver
  Serial.begin(baudRate);
  GPSSerial.begin(baudRate);
  while(!GPSSerial);
  
  // uncomment this line to turn on RMC (recommended minimum) and GGA (fix data) including altitude
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  
  // Set the update rate
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ); // 1 Hz update rate
}

// Call on Loop
// Returns false if we failed to parse a NMEA sentence from GPS
bool gps_parse_new_data(char* buf = NULL) {
  #ifdef DEBUG_FUNCTION
  debug_log("Function", "GPS Parse New Data");
  #endif
  // read data from the GPS in the 'main loop'
  char c = GPS.read();
  // if you want to debug, this is a good time to do it!
  #ifdef DEBUG_NMEA
  //if (c) debug_log("GPSECHO", String(c));
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
