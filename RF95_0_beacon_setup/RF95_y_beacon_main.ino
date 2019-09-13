// SET APP SETTINGS HERE
//#define BROADCAST_ONLY_WITH_FIX
#define READ_FROM_SERIAL

BeaconData beaconData; // Storage for beacon data

// Helper Functions

void beacon_interpret_serial(char* in, uint8_t len) {
  // If it's a message: do this
  memcpy(beaconData.msg, in, len < MAX_MSG_LEN ? len : MAX_MSG_LEN);
  // Else
  // DO SOME OTHER THING

  // Note: Can interpret commands from input stream aside from just messages.
}

void beacon_input_stream() {

  #ifdef READ_FROM_SERIAL
  // Read from serial
  if(serial_read_message(serial_in_buf, MAX_SERIAL_IN_LEN)) { // populate beacon data struct with message from serial
    // DO SOMETHING
    beacon_interpret_serial(serial_in_buf, MAX_SERIAL_IN_LEN);
  } else {
    // DO SOME CORRECTION
  }
  #endif
  
  // NOTE: GPS must be parsed and logged onto the GPS struct every frame.
  // If cannot parse new data, don't do anything.
  if(gps_parse_new_data()) {
    // DO SOMETHING
  } else {
    // DO SOME CORRECTION
  }
}

// Broadcast generated data payload over LoRa
void broadcast_beacon_data() {

  // Populating beacon data
  update_beacon_data(&beaconData, &GPS); 
  
  // For Android App or Debug
  print_beacon_data(&beaconData, serial_out_buf); // print beacon data to serial.

  #ifdef DEBUG_BEACON
  debug_log("GPS FIX" , beaconData.fix ? "true" : "false" );
  #endif 

  #ifdef BROADCAST_ONLY_WITH_FIX
  // If no fix, don't do anything.
  if (!beaconData.fix) 
    return;
  #endif

  // Actually send it.
  #ifdef DEBUG_BEACON
  debug_log("DATA", String((char*)&beaconData));
  #endif
  lora_send(&beaconData);
  #ifdef DEBUG_BEACON
  debug_log("MAX_MSG_LEN", String(MAX_MSG_LEN));
  #endif
}

// BEACON USE CASE

void beacon_setup() {

  // Clear Beacon Data
  clear_buf((uint8_t*)&beaconData); // Hacky Clearing of beacon data
  
  Serial.begin(SERIAL_BAUDRATE);
  // Initialize GPS
  gps_init();
  
  // Initialize LoRa
  lora_init();
}

void beacon_loop() {
  beacon_input_stream(); 
  broadcast_loop(broadcast_beacon_data);
}
