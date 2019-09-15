#define CSV_FORMAT  "%d,%d,%d,%d,%d,%d,%f,%f,%d,%s"//NICE_FORMAT (Readable format for terminal) or CSV_FORMAT (Can be parsed with Beacon's Android App) to toggle
//#define NICE_FORMAT "Time: %2d:%2d:%2d.%d\nDate: %2d/%2d/20%2d\nFix: %d Quality:%d\nMessage: %s\nSending Beacon Signal...\nLocation: %f, %f\nAltitude: %f"

// Populate beacon data struct with GPS values
void update_beacon_data(struct BeaconData* data, struct Adafruit_GPS* gps_in) {
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

void clear_beacon_data(struct BeaconData* data) {
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
void print_beacon_data(struct BeaconData* data, char* buf) {

  if(!Serial) return;
  
  #ifdef NICE_FORMAT
  sprintf(buf, 
    NICE_FORMAT,
    data->hour, data->minute, data->seconds, GPS.milliseconds, 
    data->day, data->month, data->year, 
    (int)data->fix, (int)data->fixq, data->msg,
    data->latitude, data->longitude, data->altitude);
  #endif
  
  #ifdef CSV_FORMAT // change to Single serial command, change to csv format for easy android parsing
  sprintf(buf,CSV_FORMAT,
    data->year, data->month, data->day, data->hour, data->minute, data->seconds, // 18 chars
    data->latitude, data->longitude, data->fix, // dddmm.mm,dddmm.mm,fff.ffff, 27
    data->fix ? data->msg : "NO GPS LOCK"); // 161
  #endif
  
  Serial.println(buf);

  #ifdef DEBUG_FUNCTION
  debug_log("Serial Print Beacon Data" , String(buf) );
  #endif
}

// Returns true if GPS has a fix.
bool gps_has_fix(BeaconData* data) {
  return data->fix && data->fixq > 0;
}
