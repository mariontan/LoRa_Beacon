// MAIN LIFE CYCLE

void setup() {
  // Initialize Timer
  broadcast_time_init();
  
  // Actual Setup
  #ifdef RF_BEACON_MODE
  beacon_setup();
  #endif

  #ifdef RF_TEST_MODE
  test_setup();
  #endif  
}

void loop() { 

  #ifdef RF_BEACON_MODE
  beacon_loop();
  #endif

  #ifdef RF_TEST_MODE
  test_loop();
  #endif
}
