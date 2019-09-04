#include <SPI.h>            //Arduino serial interface with the LoRa module and SD logger

// SET DEFAULT MESSAGE SETTINGS HERE
#define MAX_SERIAL_OUT_LEN 640
#define MAX_SERIAL_IN_LEN 161

// DATA BUFFERS
char serial_out_buf[MAX_SERIAL_OUT_LEN]; // Buffer for formatting strings for serial output.
char serial_in_buf[MAX_SERIAL_IN_LEN]; // Buffer for serial input.

// Populate message buffer from Serial
bool serial_read_message(char* buf, uint8_t max_len) {
  // If there's nothing to read, do nothing.
  if(Serial.available() <= 0) { return false; }
  
  // Don't read unless there you know there is data. Stop reading if one less than the size of the array.
  // Read a character. Store it. Increment where to write next
  byte i = 0;
  while (Serial.available() > 0 && i < max_len-1) { buf[i++] = Serial.read(); } 
  buf[i] = 0; // Null terminate the string
  
  #ifdef DEBUG_SERIAL
  debug_log("Serial Read Message", "'" + String(buf) + "'");
  #endif
  
  return true;
}
