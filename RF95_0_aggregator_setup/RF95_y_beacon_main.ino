// SET APP SETTINGS HERE
//#define PROCESS_ONLY_WITH_FIX

BeaconData beaconData; // Storage for beacon data

// Helper Functions

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

void beacon_setup() 
{  
  // Clear Beacon Data
  clear_buf((uint8_t*)&beaconData); // Hacky Clearing of beacon data
  
  Serial.begin(SERIAL_BAUDRATE);

  // Initialize LoRa
  // while (!lora_init());
  lora_init();
  
  // TODO: Put here functions that should not be done until lora is initialized.
}

void beacon_loop()
{
  // Wait for a message addressed to us from the client
  // Get header info: from (sender), id(TODO: research what this is)
  uint8_t len, from, id;
  if(!lora_recv(&beaconData, sizeof(beaconData), &len, &from, &id))
    return;
  
  debug_log("Fix", String(beaconData.fix ? "True" : "False"));
  // Process data.
  process_data(from, &beaconData);
  // Clear Beacon Data
  //clear_buf((uint8_t*)&beaconData); // Hacky Clearing of beacon data
}
