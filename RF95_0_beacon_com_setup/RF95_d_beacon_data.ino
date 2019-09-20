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

// Returns true if GPS has a fix.
bool gps_has_fix(BeaconData* data) {
  return data->fix && data->fixq > 0;
}
