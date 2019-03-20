// SET APP SETTINGS HERE
//#define PROCESS_ONLY_WITH_FIX

BeaconData beaconData; // Storage for beacon data

// Helper Functions

// Process data received from senderID
void process_data(uint8_t from, struct BeaconData* data) {
  
  #ifdef PROCESS_ONLY_WITH_FIX
  // If there's no GPS fix from received data, do nothing.
  if(!gps_has_fix(data)) return;
  #endif
  
  Serial.print("#");
  print_lora_info(serial_out_buf,from,RF_DRIVER.lastRssi()); // Print LoRa Node and RSSI
  print_beacon(serial_out_buf, data); // Print Beacon GPS and Message
  Serial.println("*");
}

void beacon_setup() 
{  
  Serial.begin(SERIAL_BAUDRATE);
  while (!Serial); // Wait for serial port to be available

  // Initialize LoRa
  if (!lora_init())
    return;

  // TODO: Put here functions that should not be done until lora is initialized.
}

void beacon_loop()
{
  // Wait for a message addressed to us from the client
  // Get header info: from (sender), id(TODO: research what this is)
  uint8_t len, from, id;

  // If no data received, do nothing.
  if(!lora_recv(&beaconData, &len, &from, &id)) 
    return;

  // Process data.
  process_data(from, &beaconData);
}
