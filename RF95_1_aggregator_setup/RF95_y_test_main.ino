void test_loop() {
  uint8_t len, from, id;

  if(!lora_recv(lora_buf, &len, &from, &id)) 
    return;

  char data[len];
  
  // Map buffer onto data
  memcpy(data, lora_buf, len);

  debug_log("Received Message", String(data));
  
}
