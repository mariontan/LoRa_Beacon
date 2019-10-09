#define SERIAL_BAUDRATE 9600
#define LORA_TX
#define LORA_RX
#define RF_BEACON_ID 3 // ID for this beacon node.
#define RF_AGGREGATOR_ID 20 // Aggregator node's ID as receiver node ID.

// SET DEFAULT LORA NODE IDs HERE
#define RF_THIS_ID    RF_BEACON_ID 
#define RF_DESTINATION_ID 5//RF_AGGREGATOR_ID 

#define RF_BEACON_MODE
//#define RF_TEST_MODE
//#define RF_DUMMY_MODE

// SET DEBUG HERE
//#define DEBUG_BEACON
//#define DEBUG_NMEA
#define DEBUG_LORA
