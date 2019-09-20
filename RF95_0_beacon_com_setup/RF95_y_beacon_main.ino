// SET APP SETTINGS HERE
//#define BROADCAST_ONLY_WITH_FIX
#define READ_FROM_SERIAL
#define CHAT_MODE
BeaconData beaconData; // Storage for beacon data
BeaconData dataReceived ;

// Helper Functions

void beacon_interpret_serial(char* in, uint8_t len) {
  // If it's a message: do this
  memcpy(beaconData.msg, in, len < MAX_MSG_LEN ? len : MAX_MSG_LEN);
  // Else
  // DO SOME OTHER THING

  // Note: Can interpret commands from input stream aside from just messages.
}

// Broadcast generated data payload over LoRa
void broadcast_beacon_data() {

  // Populating beacon data
  update_beacon_data(&beaconData, &GPS); 
  
  // For Android App or Debug
  to_string_beacon(serial_out_buf, &beaconData);
  Serial.println(serial_out_buf);

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

void beacon_input_stream() {

  // NOTE: GPS must be parsed and logged onto the GPS struct every frame.
  // If cannot parse new data, don't do anything.
  if(gps_parse_new_data()) {
    // DO SOMETHING
  } else {
    // DO SOME CORRECTION
  }
  
  #ifdef READ_FROM_SERIAL
  // Read from serial
  if(serial_read_message(serial_in_buf, MAX_SERIAL_IN_LEN)) { // populate beacon data struct with message from serial
    // DO SOMETHING
    beacon_interpret_serial(serial_in_buf, MAX_SERIAL_IN_LEN);
    #ifdef CHAT_MODE
      broadcast_beacon_data();
    #endif
  } else {
    // DO SOME CORRECTION
    
    #ifdef CHAT_MODE
      com_recv(); 
    #endif
  }
  #endif
  
}

// Process data received from senderID
void process_data(uint8_t from, struct BeaconData* data) {
  if(!Serial) return; // Wait for serial port to be available
  
  #ifdef PROCESS_ONLY_WITH_FIX
  // If there's no GPS fix from received data, do nothing.
  if(!gps_has_fix(data)) return;
  #endif
    
  Serial.print("#");
  to_string_lora_info(serial_out_buf,from,RF_DRIVER.lastRssi()); // Print LoRa Node and RSSI
  Serial.print(serial_out_buf);
  to_string_beacon(serial_out_buf, data); // Print Beacon GPS and Message
  Serial.print(serial_out_buf);
  Serial.println("*");
}

void com_recv()
{
  // Wait for a message addressed to us from the client
  // Get header info: from (sender), id(TODO: research what this is)
  uint8_t len, from, id;
  if(!lora_recv(&dataReceived, sizeof(dataReceived), &len, &from, &id))
    return;
  
  debug_log("Fix", String(dataReceived.fix ? "True" : "False"));
  // Process data.
  process_data(from, &dataReceived);
  // Clear Beacon Data
  //clear_buf((uint8_t*)&dataReceived); // Hacky Clearing of beacon data
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
  #ifndef CHAT_MODE
  com_recv(); 
  broadcast_loop(broadcast_beacon_data);
  #endif
}
