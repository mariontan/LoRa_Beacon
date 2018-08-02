#include <SPI.h>            //Arduino serial interface with the LoRa module and SD logger
#include <RHReliableDatagram.h>     //Manager library used for addressed, unreliable messages
#include <RH_RF95.h>        //Library used to send messages using LoRa
#include <string.h>
#include <ctype.h>

#define RF_FREQUENCY  451.00
#define RF_AGGREGATOR_ID 0 
#define RF_NODE_ID    1

RH_RF95 rf95d;                                //Singleton instance of the radio driver
RHReliableDatagram rf95m(rf95d, RF_NODE_ID);  //This class manages message delivery and reception

uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
uint8_t tx_buf[RH_RF95_MAX_MESSAGE_LEN];
uint8_t rmctag[] = "$GPRMC";
uint8_t bcntag[] = "#LBS";                     // LoRa Beacon Signal tag
int byteGPS;
int check = 0;
int count = 0;

void setup() {  
  Serial.begin(57600);              // Set the hardware serial port to 57600, baudrate of the GPS receiver
  while (!Serial);                  // Wait for serial port to be available
  /*--Initializing LoRa module--*/

  if (rf95m.init()) {
    //Adjust Frequency
    rf95d.setFrequency(RF_FREQUENCY);
  
    //Adjust Power to 23 dBm
    rf95d.setTxPower(23, false);
  
    // This is our Node ID
    rf95m.setThisAddress(RF_NODE_ID);
    rf95m.setHeaderFrom(RF_NODE_ID);
  
    // Where we're sending packet
    rf95m.setHeaderTo(RF_AGGREGATOR_ID);
    //rf95m.setRetries(7);         
  }
      
  for (int i=0; i<RH_RF95_MAX_MESSAGE_LEN; i++){       // Initialize a buffer for received data
     buf[i]='\0';
  }
  for (int i=0; i<sizeof(tx_buf); i++){    // Initialize buffer for transmitted data by filling with NULL
     tx_buf[i] = 0;
  }      
}

void loop() {
  if (Serial.available()) {
    byteGPS = Serial.read();         // Read a byte of the serial port
    if (byteGPS == -1) {             // See if the port is empty yet
      delay(100);      
    } 
    else { 
      // note: there is a potential buffer overflow here!
      buf[count] = byteGPS;        // If there is serial port data, it is put in the buffer
      count++;
      if (byteGPS == 13) {            // If the received byte is = to 13, end of transmission
        // note: the actual end of transmission is <CR><LF> (i.e. 0x0D 0x0A)
//        cnt=0;
        check=0;
        // The following for loop starts at 1, because this code is clowny and the first byte is the <LF> (0x10) from the previous transmission.
        for (int i=1; i<sizeof(rmctag); i++){     // Verifies if the received command starts with $GPR
          if (buf[i] == rmctag[i-1]) {
            check++;
         }
        }
        
        if (check == 6) { // If yes, continue and send the NMEA Sentence
          for (int i=0; i<sizeof(bcntag)-1; i++) { // Copies to the 1st 4 elements of tx_buf the values of bcntag
              tx_buf[i] = bcntag[i];
          }
          for (int i=sizeof(bcntag)-1; i<sizeof(tx_buf); i++) { // Copies to tx_buf value of buf[1] onwards; buf[0] is newline
              tx_buf[i] = buf[i-sizeof(bcntag)+2];              
          }
          // Send a message to aggregator
          rf95m.sendtoWait(tx_buf, sizeof(tx_buf), RF_AGGREGATOR_ID);
          delay(1000);
       }
       count=0;                    // Reset the buffers by filling with NULL
       for (int i=0; i<sizeof(buf); i++){    
         buf[i]= 0;
       }
       for (int i=0; i<sizeof(tx_buf); i++){    
         tx_buf[i]= 0;
       }
      }
    }
  }
}
