#include <SPI.h>                  //Arduino SPI interface with the LoRa module
#include <Wire.h>                 //Arduino I2C interface with the Atlas Scientific sensors
#include <RHReliableDatagram.h>   //Manager library used for addressed, unreliable messages
#include <RH_RF95.h>              //Library used to send messages using LoRa
#include <OneWire.h>              //OneWire interface for DS18B20 temperature sensor
#include <DallasTemperature.h>    //Library for DS18B20 temperature sensor

#define RF_FREQUENCY  868.00
#define RF_GATEWAY_ID 0
#define RF_NODE_ID    1

#define DO_add 97               //default I2C ID number for EZO DO Circuit.
#define EC_add 100              //default I2C ID number for EZO EC Circuit.

#define TEMP_BUS 3              //set pin D3 for DS18B20 temperature sensor; D2 is used by RF95 for interrupt

RH_RF95 rf95d;                        //Singleton instance of the radio driver
RHReliableDatagram rf95m(rf95d, RF_NODE_ID);  //This class manages message delivery and reception

OneWire ds(TEMP_BUS);                   // on digital pin 2
DallasTemperature tsens(&ds);     // DallasTemperature object for DS18B20 sensor
//DeviceAddress waterTemp;

//byte computer_bytes_received=0;   //We need to know how many characters bytes have been received
//byte sensor_bytes_received = 0;     //We need to know how many characters bytes have been received

struct dataStruct {     //stores the sensor values in a struct for easier sending and receiving via LoRa
  float temp, diox, cdty;
} nodeData;

void setup() {
  Serial.begin(9600);               //Set the hardware serial port to 9600
  while (!Serial);                  //Wait for serial port to be available
  Wire.begin();                     //enable I2C port.
  tsens.begin();                    //Start DallasTemperature object

  /*--Initializing LoRa module--*/
  if (rf95m.init()) {
    //Adjust Frequency
    rf95d.setFrequency(RF_FREQUENCY);
  
    //Adjust Power to 23 dBm
    rf95d.setTxPower(23, false);

    // Setup BandWidth, option: 7800,10400,15600,20800,31200,41700,62500,125000,250000,500000
    //Lower BandWidth for longer distance.
    rf95d.setSignalBandwidth(125000);
  
    // Setup Spreading Factor (6 ~ 12)
    rf95d.setSpreadingFactor(7);
    
    // Setup Coding Rate:5(4/5),6(4/6),7(4/7),8(4/8) 
    rf95d.setCodingRate4(5);
  
    // This is our Node ID
    rf95m.setThisAddress(RF_NODE_ID);
    rf95m.setHeaderFrom(RF_NODE_ID);
  
    // Where we're sending packet
    rf95m.setHeaderTo(RF_GATEWAY_ID);
    //rf95m.setRetries(7); 
    }

    Wire.beginTransmission(EC_add);       //call the circuit by its ID number.
    Wire.write("sleep");                  //transmit sleep command to EC_EZO.
    Wire.endTransmission();               //end the I2C data transmission.
    Wire.beginTransmission(DO_add);       //call the circuit by its ID number.
    Wire.write("sleep");                  //transmit sleep command to DO_EZO.
    Wire.endTransmission();               //end the I2C data transmission.
    
  //Setting up the sensor interfaces
  //analogReference(DEFAULT);         //Default at 5.0V
  
    Serial.print("Clearing trash data: ");
    Serial.print(getTemp(0)); Serial.print(",");
    Serial.print(getAtlasDO()); Serial.print(",");
    Serial.println(getAtlasEC());
  
}

void loop(void) {
  const int interval = 10;
  uint8_t rx_buf[RH_RF95_MAX_MESSAGE_LEN], tx_buf[RH_RF95_MAX_MESSAGE_LEN]; // Saving dynamic memory. Since we only receive byte 'S', we limit rx_buf size. Alloted 64-bytes for tx_buf.  
//  if (rf95m.available()) {
//    uint8_t len = sizeof(rx_buf);
//    uint8_t from;
//    if (rf95m.recvfrom(rx_buf, &len, &from)) { //If trigger is received from server, send recent data or accumulated data
//      if (rx_buf[0] == 83) { //If trigger received is "S", get latest data from sensors and send them
        nodeData.temp = getTemp(0);
        nodeData.diox = getAtlasDO();
        nodeData.cdty = getAtlasEC();
        Serial.print(nodeData.temp); Serial.print(",");
        Serial.print(nodeData.diox); Serial.print(",");
        Serial.println(nodeData.cdty);       
        memcpy(tx_buf, &nodeData, sizeof(nodeData));
        rf95m.sendtoWait(tx_buf, sizeof(nodeData), RF_GATEWAY_ID);
        rf95m.waitPacketSent();
        delay(interval*1000);
//      }
//    }
//  }
}


float getAtlasEC() {
  float ec_float;
  char ec_char[48];                //we make a 48 byte character array to hold incoming data from the EC circuit.
  byte in_char = 0;                //used as a 1 byte buffer to store inbound bytes from the EC Circuit.
  byte i = 0;                      //counter used for ec_char array.
  byte code = 0;                   //used to hold the I2C response code.

  Wire.beginTransmission(EC_add);       //call the circuit by its ID number.
  Wire.write("r");                      //transmit return value command to EC_EZO.
  Wire.endTransmission();               //end the I2C data transmission.
  delay(600);                           //600 ms delay time before reading output from EC_EZO.
  
  Wire.requestFrom(EC_add, 48, 1);                                     //call the circuit and request 48 bytes (this is more than we need)
  code = Wire.read();                                                   //the first byte is the response code, we read this separately.

  while (Wire.available()) {            //are there bytes to receive.
    in_char = Wire.read();              //receive a byte.
    ec_char[i] = in_char;               //load this byte into our array.
    i += 1;                             //incur the counter for the array element.
    if (in_char == 0) {                 //if we see that we have been sent a null command.
      i = 0;                            //reset the counter i to 0.
      Wire.endTransmission();           //end the I2C data transmission.
      break;                            //exit the while loop.
    }
  }
  
  Wire.beginTransmission(EC_add);       //call the circuit by its ID number.
  Wire.write("sleep");                  //transmit sleep command to EC_EZO.
  Wire.endTransmission();               //end the I2C data transmission.
  ec_float = atof(ec_char);
  return ec_float;
}

float getAtlasDO() {
  float do_float;
  char do_char[48];                //we make a 48 byte character array to hold incoming data from the DO circuit.
  byte in_char = 0;                //used as a 1 byte buffer to store inbound bytes from the DO Circuit.
  byte i = 0;                      //counter used for ec_char array.
  byte code = 0;                   //used to hold the I2C response code.
  
  Wire.beginTransmission(DO_add);       //call the circuit by its ID number.
  Wire.write("r");                      //transmit return value command to EC_EZO.
  Wire.endTransmission();               //end the I2C data transmission.
  delay(600);                           //600 ms delay time before reading output from EC_EZO.

  Wire.requestFrom(DO_add, 48, 1);                                     //call the circuit and request 48 bytes (this is more than we need)
  code = Wire.read();                                                   //the first byte is the response code, we read this separately.
  
  while (Wire.available()) {            //are there bytes to receive.
    in_char = Wire.read();              //receive a byte.
    do_char[i] = in_char;               //load this byte into our array.
    i += 1;                             //incur the counter for the array element.
    if (in_char == 0) {                 //if we see that we have been sent a null command.
      i = 0;                            //reset the counter i to 0.
      Wire.endTransmission();           //end the I2C data transmission.
      break;                            //exit the while loop.
    }
  }
  
  Wire.beginTransmission(DO_add);       //call the circuit by its ID number.
  Wire.write("sleep");                  //transmit sleep command to EC_EZO.
  Wire.endTransmission();               //end the I2C data transmission.
  do_float = atof(do_char);
  return do_float;
}

float getTemp(int index) {
  float stemp;
  tsens.requestTemperatures();
  stemp = tsens.getTempCByIndex(index);
  return stemp;
}
