// rf95_reliable_datagram_server.pde
// -*- mode: C++ -*-
// Example sketch showing how to create a simple addressed, reliable messaging server
// with the RHReliableDatagram class, using the RH_RF95 driver to control a RF95 radio.
// It is designed to work with the other example rf95_reliable_datagram_client
// Tested with Anarduino MiniWirelessLoRa, Rocket Scream Mini Ultra Pro with the RFM95W 

#include <RHReliableDatagram.h>
#include <RH_RF95.h>
#include <SPI.h>
#include<stdlib.h>
#include<string.h>

// SET LORA NODE IDs HERE
#define RF_AGGREGATOR_ID  0

// SET DEFAULT LORA SETTINGS HERE
#define RF_FREQUENCY      433.00 
#define RF_TX_POWER 23
#define RF_USE_RFO false
#define RF_BANDWIDTH 125000
#define RF_SPREAD_FACTOR 7
#define RF_CODING_RATE 5

// SET DEFAULT SERIAL SETTINGS HERE
#define SERIAL_BAUDRATE 9600

// SET DEFAULT MESSAGE SETTINGS HERE
#define MAX_MSG_LEN 161

RH_RF95 RF_DRIVER(8,3);                                //Singleton instance of the radio driver
RHReliableDatagram RF_MESSAGING(RF_DRIVER, RF_AGGREGATOR_ID);  //This class manages message delivery and reception

struct BeaconData {     //stores the sensor values in a struct for easier sending and receiving via LoRa
  uint8_t hour, minute, seconds, year, month, day, fixq;
  char nsd, ewd;
  float latitude, longitude, altitude, hdop;
  boolean fix;
  char msg[MAX_MSG_LEN];
};

// DATA BUFFERS
BeaconData beaconData; // Storage for beacon data
char* serial_buf; // Buffer for formatting strings.

// Dont put this on the stack:
uint8_t rx_buf[RH_RF95_MAX_MESSAGE_LEN]; // LoRa Byte Array payload buffer

// Need this on Arduino Zero with SerialUSB port (eg RocketScream Mini Ultra Pro)
//#define Serial SerialUSB

// LORA INITIALIZATION

// Set RF95 Parameters
void lora_set_parameters(float frequency = RF_FREQUENCY, int8_t txPower = RF_TX_POWER, bool useRFO = RF_USE_RFO, long bandwidth = RF_BANDWIDTH, int8_t spreadFactor = RF_SPREAD_FACTOR, int8_t codingRate = RF_CODING_RATE) {
  
  //Adjust Frequency
  RF_DRIVER.setFrequency(frequency);

  //Adjust Power to 23 dBm
  RF_DRIVER.setTxPower(txPower, useRFO);

  // Setup BandWidth, option: 7800,10400,15600,20800,31200,41700,62500,125000,250000,500000
  //Lower BandWidth for longer distance.
  RF_DRIVER.setSignalBandwidth(bandwidth);

  // Setup Spreading Factor (6 ~ 12)
  RF_DRIVER.setSpreadingFactor(spreadFactor);

  // Setup Coding Rate:5(4/5),6(4/6),7(4/7),8(4/8)
  RF_DRIVER.setCodingRate4(codingRate);
}

// Set this node's ID
void lora_set_source(uint8_t nodeID) {
  // This is our Node ID
  RF_MESSAGING.setThisAddress(nodeID);
  RF_MESSAGING.setHeaderFrom(nodeID);
}

// Set up driver and messaging service.
void lora_setup() {
  lora_set_parameters();
  lora_set_source(RF_AGGREGATOR_ID);
}

// Initialize LoRa components.
bool lora_init() {
  if (!RF_MESSAGING.init()){
    Serial.println("init failed");
    return false;
  }
  
  lora_setup();
  return true;
}

// RX HELPER FUNCTIONS

// Receive byte array data from LoRa
bool lora_recv(uint8_t* buf, uint8_t* from, uint8_t* id){
// If no message is available, ignore and wait.
  if (!RF_MESSAGING.available())
    return false;
  
  uint8_t len = sizeof(buf);

  // If failed to received message from buffer, ignore and wait.
  if (!RF_MESSAGING.recvfromAck(buf, &len, from, NULL, id))
    return false;

  return true;
}

// Receive  Beacon Data from LoRa
bool recv_beacon_data(BeaconData* data, uint8_t* from, uint8_t* id) {

  if(!lora_recv(rx_buf, from, id)) 
    return false;
    
  // Map buffer onto beacon data struct.
  memcpy(data, rx_buf, sizeof(*data));
  
  return true;
}

// Receive data from LoRa
bool recv_data(uint8_t* from, uint8_t* id){
  return recv_beacon_data(&beaconData, from, id);
}

// PROCESS DATA

// Print LoRa message from ID and RSSI.
void serial_print_lora_info(char* buf, uint from, uint rssi) {
  sprintf(buf,"%d;%d;",from, rssi);
  Serial.print(buf); 
}

// Print beacon GPS and message to serial.
void serial_print_beacon(char* buf, BeaconData* data) {
  // Reconstruct UTC date and time
  // Print delimited sentence
  sprintf(buf, 
    "%02d%02d%02d,%02d%02d%02d;%f;%s;%f;%s;%f;%f;%s;", 
    data->day, data->month, data->year, 
    data->hour, data->minute, data->seconds,
    data->latitude, data->nsd, data->longitude, data->ewd, 
    data->altitude, data->hdop, data->msg);
  Serial.print(buf); 
}

// Returns true if GPS has a fix.
bool gps_has_fix(BeaconData* data) {
  return data->fix && data->fixq > 0;
}

// Print data to Serial Port
void serial_print_data(uint from, uint rssi, BeaconData* data) {
  
  Serial.print("#");
  serial_print_lora_info(serial_buf,from,rssi); // Print LoRa Node and RSSI
  serial_print_beacon(serial_buf, data); // Print Beacon GPS and Message
  Serial.println("*");
}

// Process data received from senderID
void process_data(uint8_t senderID) {
  // If there's no GPS fix from received data, do nothing.
  if(!gps_has_fix(&beaconData))
    return;
    
  // Write all lora metadata and beacon data to serial
  serial_print_data(senderID, RF_DRIVER.lastRssi(), &beaconData);
}

// ARDUINO LIFE CYCLE

void setup() 
{  
  Serial.begin(SERIAL_BAUDRATE);
  while (!Serial) ; // Wait for serial port to be available

  // Initialize LoRa
  if (!lora_init())
    return;

  // TODO: Put here functions that should not be done until lora is initialized.
}

void loop()
{
  // Wait for a message addressed to us from the client
  // Get header info: from (sender), id(TODO: research what this is)
  uint8_t from, id;

  // If no data received, do nothing.
  if(!recv_data(&from, &id)) 
    return;

  // Process data.
  process_data(from);
}
