
// SET DEFAULT LORA NODE IDs HERE
#define RF_AGGREGATOR_ID 0 // Aggregator node's ID as receiver node ID.

// MAIN LIFE CYCLE

void setup() {
  // Initialize Timer
  broadcast_time_init();
  
  // Actual Setup
  if(!beacon_setup())return;
  //if(!test_setup())return;
  
  lora_set_destination(RF_AGGREGATOR_ID);
}

void loop() { 
  beacon_loop();
  //test_loop();
}
