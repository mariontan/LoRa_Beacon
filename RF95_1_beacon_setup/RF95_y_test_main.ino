// SIMPLE USE CASE

uint8_t simple_index = 0;

void broadcast_test_data() {
  sprintf((char*)lora_buf, "%d", simple_index++);
  #ifdef DEBUG_FUNCTION
  debug_log("Sending", lora_buf);
  #endif
  lora_send(lora_buf);
}

bool test_setup() {
  if (!lora_init()) return false; // Initialize LoRa
  return true;
}

void test_loop() {
  broadcast_loop(broadcast_test_data);
}
