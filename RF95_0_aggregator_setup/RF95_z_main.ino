void setup() {
  beacon_setup();
}

void loop() {
  
  #ifdef RF_BEACON_MODE
  beacon_loop();
  #endif

  #ifdef RF_TEST_MODE
  test_loop();
  #endif
}
