// SIMPLE USE CASE
#define DUMMY_TEST

uint8_t simple_index = 0;

void broadcast_test_data() {
  #ifdef DUMMY_TEST
  sprintf((char*)lora_buf, "160919,044139;0.000020;121.076157;5.000000;4.010000;Help me please;0;0");
  #else
  sprintf((char*)lora_buf, "%d", simple_index++);
  #endif
  #ifdef DEBUG_FUNCTION
  debug_log("Sending",(char*)lora_buf); 
  #endif
  Serial.println((char*)lora_buf);
  lora_send(lora_buf);
}

void test_setup() {
  lora_init();
}

void test_loop() {
  broadcast_loop(broadcast_test_data);
}
