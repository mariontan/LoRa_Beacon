
void test_loop() {
  uint8_t len, from, id;
  if(!lora_recv(lora_buf, RH_RF95_MAX_MESSAGE_LEN, &len, &from, &id)) 
    return;
  debug_log("Received Message", String((char*) lora_buf));
}
