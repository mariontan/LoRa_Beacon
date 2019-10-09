#define BEACON_META_LEN 24
#define MAX_MSG_LEN (RH_RF95_MAX_MESSAGE_LEN - BEACON_META_LEN - 4) // 161

// 24 metadata bytes + MAX_MSG_LEN bytes
struct BeaconData {     //stores the sensor values in a struct for easier sending and receiving via LoRa
  uint8_t hour, minute, seconds, year, month, day, fixq; // 1 byte each = 7 bytes
  boolean fix; // 1 byte
  float latitude, longitude, altitude, hdop; // 4 bytes each = 16 bytes
  char msg[MAX_MSG_LEN]; // MAX_MSG_LEN bytes
};

// Format beacon GPS and message to serial.
void to_string_beacon(char* buf, struct BeaconData* data, uint8_t destination = RF_DESTINATION_ID, uint8_t source = RF_THIS_ID) {
  
  // Reconstruct UTC date and time
  // Print delimited sentence
  //DDMMYY,hhmmss;lat;lon;alt;hdop;msg;fixQuality;fixIsTrue 
  // 160919,044139;0.000020;121.076157;5.000000;4.010000;Help me please;0.000000;0
  sprintf(buf, 
    "%02d%02d%02d,%02d%02d%02d;%f;%f;%f;%f;%s;%d;%d;%d;%d", 
    data->day, data->month, data->year, 
    data->hour, data->minute, data->seconds,
    data->latitude, data->longitude,
    data->altitude, data->hdop, data->msg,
    data->fixq, data->fix, destination, source);
}
