// Set DEBUG SETTINGS HERE
//#define MAX_DEBUG_LEN 640
//#define DEBUG_FUNCTION
//#define DEBUG_BUFFER
//#define DEBUG_SERIAL

#ifdef MAX_DEBUG_LEN
char debug_buf[MAX_DEBUG_LEN]; // Buffer for debugger
#endif

// HELPERS
void debug_log(String tag, String log_buf) {
  
  if(!Serial) return; // Wait for serial port to be available
  #ifdef MAX_DEBUG_LEN
  String buf = tag + ": " + log_buf;
  buf.toCharArray(debug_buf, MAX_DEBUG_LEN);
  Serial.println(debug_buf);
  #endif
}
