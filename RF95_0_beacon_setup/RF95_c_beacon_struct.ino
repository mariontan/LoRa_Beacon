#define BEACON_META_LEN 24
#define MAX_MSG_LEN (RH_RF95_MAX_MESSAGE_LEN - BEACON_META_LEN - 4) // 161

// 24 metadata bytes + MAX_MSG_LEN bytes
struct BeaconData {     //stores the sensor values in a struct for easier sending and receiving via LoRa
  uint8_t hour, minute, seconds, year, month, day, fixq; // 1 byte each = 7 bytes
  float latitude, longitude, altitude, hdop; // 4 bytes each = 16 bytes
  boolean fix; // 1 byte
  char msg[MAX_MSG_LEN]; // MAX_MSG_LEN bytes
};

// Print beacon GPS and message to serial.
void print_beacon(char* buf, struct BeaconData* data) {
  // Reconstruct UTC date and time
  // Print delimited sentence
  sprintf(buf, 
    "%02d%02d%02d,%02d%02d%02d;%f;%f;%f;%f;%s;", 
    data->day, data->month, data->year, 
    data->hour, data->minute, data->seconds,
    data->latitude, data->longitude,
    data->altitude, data->hdop, data->msg);
  Serial.print(buf); 
}
