#include <SPI.h>            //Arduino serial interface with the LoRa module and SD logger
#include <RHReliableDatagram.h>     //Manager library used for addressed, unreliable messages
#include <RH_RF95.h>        //Library used to send messages using LoRa
#include <string.h>
#include <ctype.h>

#define RF_FREQUENCY  433.00
#define RF_AGGREGATOR_ID 0 
#define RF_NODE_ID    6

RH_RF95 rf95d(8,3);                                //Singleton instance of the radio driver
RHReliableDatagram rf95m(rf95d, RF_NODE_ID);  //This class manages message delivery and reception

uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
uint8_t tx_buf[RH_RF95_MAX_MESSAGE_LEN];
uint8_t rmctag[] = "$GPRMC";
uint8_t bcntag[] = "#LBS";                     // LoRa Beacon Signal tag
int byteGPS;
int check = 0;
int count = 0;
int led = 13;                                 // Use IO LED pin 13 of the Featherwing for to indicate send success

void setup() {  
  Serial.begin(9600);              // Set the hardware serial port to 9600, baudrate of the GPS receiver
  Serial1.begin(9600);
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
    rf95m.setHeaderTo(RF_AGGREGATOR_ID);
    //rf95m.setRetries(7);         
  }
      
  for (int i=0; i<RH_RF95_MAX_MESSAGE_LEN; i++){       // Initialize a buffer for received data
     buf[i]='\0';
  }
  for (int i=0; i<sizeof(tx_buf); i++){    // Initialize buffer for transmitted data by filling with NULL
     tx_buf[i] = 0;
  }
  pinMode(led, OUTPUT);                     // Setup IO pin 13 to use the LED of the Featherwing
}

void loop() {
  if (Serial1.available()) {
    byteGPS = Serial1.read();         // Read a byte of the serial port
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
          Serial.println((char*)tx_buf);
          // Send a message to aggregator
          if (rf95m.sendtoWait(tx_buf, sizeof(tx_buf), RF_AGGREGATOR_ID)) digitalWrite(led, HIGH);  // Indicator LED if sendtoWait succeeds
          delay(500);
       }
      count=0;                    // Reset the buffers by filling with NULL
      for (int i=0; i<sizeof(buf); i++){    
        buf[i]= 0;
      }
      for (int i=0; i<sizeof(tx_buf); i++){    
        tx_buf[i]= 0;
      }       
      digitalWrite(led, LOW);
      }
    }
  }
}
