#define BROADCAST_INTERVAL_MILLIS 2000 // DURATION BETWEEN BROADCASTS

// LOOP
uint32_t last_broadcast_millis;

uint32_t get_duration(uint32_t timer_end, uint32_t timer_start) {
  // If the current time is greater than the last broadcast time, it still hasn't looped around. Get the duration normally.
  // If the last broadcast time is greater than the current time, current time has looped around. Get the looped around duration.
  return (timer_start <= timer_end) ? timer_end - timer_start : timer_end + (UINT32_MAX - timer_start);
}

void broadcast_time_init() {
  last_broadcast_millis = millis() - BROADCAST_INTERVAL_MILLIS;
  #ifdef DEBUG_FUNCTION
  debug_log("Function", "Broadcast Time Init: " + String(last_broadcast_millis));
  #endif
}

void broadcast_time_stamp() {
  last_broadcast_millis = millis(); // reset the last_broadcast_millis
  #ifdef DEBUG_FUNCTION
  debug_log("Function", "Broadcast Time Stamp: " + String(last_broadcast_millis));
  #endif
}

// This service waits approximately every 30 seconds or so in between broadcasts.
// Returns true if it's still waiting before it can broadcast again.
// Returns false if it's done waiting before it can broadcast again.
bool is_waiting_broadcast(uint32_t interval = BROADCAST_INTERVAL_MILLIS) {
  return (get_duration(millis(), last_broadcast_millis) <= interval);
}

// broadcast: the function for broadcasting
// interval: time between broadcasts
void broadcast_loop(void (*broadcast)(), uint32_t interval = BROADCAST_INTERVAL_MILLIS) { 
  // If cannot broadcast, wait.
  if(is_waiting_broadcast()) 
    return;
  (*broadcast)();
  broadcast_time_stamp();
}
