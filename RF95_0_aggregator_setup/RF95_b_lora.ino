#include <RHReliableDatagram.h>     //Manager library used for addressed, unreliable messages
#include <RH_RF95.h>        //Library used to send messages using LoRa

// LoRa Byte Array payload buffer
uint8_t lora_buf[RH_RF95_MAX_MESSAGE_LEN]; 

// SET DEFAULT LORA SETTINGS HERE
#define RF_FREQUENCY  433.00
#define RF_TX_POWER 23
#define RF_USE_RFO false
#define RF_BANDWIDTH 125000
#define RF_SPREAD_FACTOR 7
#define RF_CODING_RATE 5
#define RF_RETRIES 0
#define RF_CAD_TIMEOUT 4000
#define RF_TIMEOUT 4000

// CONSTANTS FOR SF AND BW USE CASES
#define RF_SPREAD_FACTOR_7 7
#define RF_SPREAD_FACTOR_8 8
#define RF_SPREAD_FACTOR_10 10
#define RF_SPREAD_FACTOR_12 12

#define RF_BANDWIDTH_125KHZ 125000
#define RF_BANDWIDTH_62_5KHZ 62500

// LORA-GPS SETUP
RH_RF95 RF_DRIVER(8, 3);                          //Singleton instance of the radio driver
RHReliableDatagram RF_MESSAGING(RF_DRIVER, RF_THIS_ID);  //This class manages message delivery and reception
uint8_t rf_destination;
uint8_t this_len, this_from, this_id;
  
// LORA INITIALIZATION

// Set RF95 Parameters
void lora_set_parameters(float frequency = RF_FREQUENCY, int8_t txPower = RF_TX_POWER, bool useRFO = RF_USE_RFO, long bandwidth = RF_BANDWIDTH, int8_t spreadFactor = RF_SPREAD_FACTOR, int8_t codingRate = RF_CODING_RATE, int8_t cadTimeout = RF_CAD_TIMEOUT) {

  #ifdef DEBUG_FUNCTION
  debug_log("Function", "lora_set_parameters");
  #endif
  //Adjust Frequency
  RF_DRIVER.setFrequency(frequency);
  debug_log("Frequency", String(frequency) );

  //Adjust Power to 23 dBm
  RF_DRIVER.setTxPower(txPower, useRFO);
  debug_log("TX power", String(txPower) );
  debug_log("Use RFO ", String(useRFO) );

  // Setup BandWidth, option: 7800,10400,15600,20800,31200,41700,62500,125000,250000,500000
  //Lower BandWidth for longer distance.
  RF_DRIVER.setSignalBandwidth(bandwidth);
  debug_log("Signal Bandwidth", String(bandwidth) );
  
  // Setup Spreading Factor (6 ~ 12)
  RF_DRIVER.setSpreadingFactor(spreadFactor);
  debug_log("Spread Factor", String(spreadFactor) );

  // Setup Coding Rate:5(4/5),6(4/6),7(4/7),8(4/8)
  RF_DRIVER.setCodingRate4(codingRate);
  debug_log("Coding Rate", String(codingRate) );

  RF_DRIVER.setCADTimeout(cadTimeout);
  debug_log("CAD Timeout", String(cadTimeout) );
}

void lora_default_parameters(int caseID = 0) {

  switch(caseID) {
    case 1:
      lora_set_parameters(RF_FREQUENCY,RF_TX_POWER,RF_USE_RFO, RF_BANDWIDTH_125KHZ, RF_SPREAD_FACTOR_7);
    break;
    case 2:
      lora_set_parameters(RF_FREQUENCY,RF_TX_POWER,RF_USE_RFO, RF_BANDWIDTH_125KHZ, RF_SPREAD_FACTOR_8);
    break;
    case 3:
      lora_set_parameters(RF_FREQUENCY,RF_TX_POWER,RF_USE_RFO, RF_BANDWIDTH_125KHZ, RF_SPREAD_FACTOR_10);
    break;
    case 4:
      lora_set_parameters(RF_FREQUENCY,RF_TX_POWER,RF_USE_RFO, RF_BANDWIDTH_62_5KHZ, RF_SPREAD_FACTOR_10);
    break;
    case 5:
      lora_set_parameters(RF_FREQUENCY,RF_TX_POWER,RF_USE_RFO, RF_BANDWIDTH_125KHZ, RF_SPREAD_FACTOR_12);
    break;
    case 6:
      lora_set_parameters(RF_FREQUENCY,RF_TX_POWER,RF_USE_RFO, RF_BANDWIDTH_62_5KHZ, RF_SPREAD_FACTOR_12);
    break;
    default:
      lora_set_parameters();
    break;
  }
}

// Set this node's ID
void lora_set_source(uint8_t nodeID) {
  debug_log("Lora Set Source",String(nodeID));
  // This is our Node ID
  RF_MESSAGING.setThisAddress(nodeID);
  RF_MESSAGING.setHeaderFrom(nodeID);
}

// TX HELPER FUNCTIONS

#ifdef LORA_TX

// Set destination node's ID
void lora_set_destination(uint8_t nodeID, uint8_t retries = RF_RETRIES, uint8_t timeOut = RF_TIMEOUT) {
  debug_log("Lora Set Destination",String(nodeID) + " <- " + String(retries) + " retries");
  // Where we're sending packet
  rf_destination = nodeID;
  RF_MESSAGING.setHeaderTo(nodeID);
  if(retries > 0)
    RF_MESSAGING.setRetries(retries);
  if(timeOut > 0)
    RF_MESSAGING.setTimeout(timeOut);
}

// Send byte array payload over LoRa
template<typename T> void lora_send(T* payload) {
  lora_send(payload, sizeof(*payload), rf_destination);
}

// Send byte array payload over LoRa
template<typename T> void lora_send(T* payload, uint8_t len, uint8_t address) {
  debug_log("LoRa Send" , String(len) + " bytes to node #" + String(address) );
  RF_MESSAGING.sendtoWait((uint8_t*)payload, len, address);// send buffered data to aggregator
  RF_MESSAGING.waitPacketSent(); // wait until properly sent
}

#endif

// RX HELPER FUNCTIONS

#ifdef LORA_RX

// Receive byte array data from LoRa
template<typename T> bool lora_recv(T* buf, uint8_t buf_size, uint8_t *len, uint8_t *from, uint8_t *id){

  #ifdef DEBUG_FUNCTION
  debug_log("Function", "lora_recv");
  #endif
  
  // If no message is available, ignore and wait.
  if (!RF_MESSAGING.available()) {
    delay(1);
    return false;
  }

  #ifdef DEBUG_LORA
  debug_log("LORA RECEIVE", "Available");
  #endif
  
  // If failed to received message from buffer, ignore and wait.
  if (!RF_MESSAGING.recvfromAck((uint8_t*)buf, len, from, NULL, id))
    return false;
  
  #ifdef DEBUG_LORA
  debug_log("Copied Size", String(buf_size));
  debug_log("Receiving From", String(*from));
  debug_log("Receiving Size in Bytes", String(*len));
  debug_log("Receiving ID", String(*id));
  #endif

  return true;
}

// Print LoRa message from ID and RSSI.
void print_lora_info(char* buf, uint from, uint rssi) {
  sprintf(buf,"%d;%d;",from, rssi);
  Serial.print(buf); 
}
#endif


// Set up driver and messaging service.
void lora_setup() {
  
  #ifdef DEBUG_FUNCTION
  debug_log("Function", "Lora Setup");
  #endif
  
  lora_set_parameters();
  lora_set_source(RF_THIS_ID);

  #ifdef RF_DESTINATION_ID
  lora_set_destination(RF_DESTINATION_ID);
  #endif
}

// Initialize LoRa components.
bool lora_init() {
  
  #ifdef DEBUG_FUNCTION
  debug_log("Function", "Lora Init");
  #endif
  
  while (!RF_MESSAGING.init()){
    debug_log("ERROR", "lora init failed");
  }
  
  lora_setup();
  return true;
}
