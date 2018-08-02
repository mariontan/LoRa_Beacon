/*
  Upload Data to IoT Server ThingSpeak (https://thingspeak.com/):
  Support Devices: LG01 
  Example sketch showing how to get data from remote LoRa node, 
  Then send the value to IoT Server
  It is designed to work with the other sketch dht11_client. 
` modified 24 11 2016
  by Edwin Chen <support@dragino.com>
  Dragino Technology Co., Limited
*/

#include <SPI.h>
#include <RHReliableDatagram.h>   //Manager library used for addressed, unreliable messages
#include <RH_RF95.h>
#include <Bridge.h>
#include <Console.h>
#include <Process.h>
#include <stdio.h>
#include <stdlib.h>

#define RF_FREQUENCY  868.00
#define RF_GATEWAY_ID 0

//If you use Dragino IoT Mesh Firmware, uncomment below lines.
//For product: LG01. 
#define BAUDRATE 115200

RH_RF95 rf95d;                        //Singleton instance of the radio driver
RHReliableDatagram rf95m(rf95d, RF_GATEWAY_ID);  //This class manages message delivery and reception

struct dataStruct {     //stores the sensor values in a struct for easier sending and receiving via LoRa
  float temp, diox, cdty;
} nodeData;

//char talkBackAPIKey[] = "NAR06FNGEHO8527L";
//char talkBackID[] = "27268";
//const int checkTalkBackInterval = 10 * 1000;    // Time interval in milliseconds to check TalkBack (number of seconds * 1000 = interval)
int HEART_LED=A2;

void setup() {

    Bridge.begin(BAUDRATE);
    Console.begin(); 
//    while(!Console);
    
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
      rf95m.setThisAddress(RF_GATEWAY_ID);
      rf95m.setHeaderFrom(RF_GATEWAY_ID);
      //rf95m.setRetries(7); 
    }
    
    else Console.println("init failed");    
    Console.println("LoRa Gateway Example  --");
    Console.println("    Upload Single Data to ThinkSpeak");
    
    pinMode(HEART_LED, OUTPUT);
}

void loop() {
//  checkTalkBack();
//  delay(checkTalkBackInterval);
  if (rf95m.available()) {    
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
    uint8_t from;
    int8_t rssi = rf95d.lastRssi();
    if (rf95m.recvfromAck(buf, &len, &from)) { //If trigger is received from server, send recent data or accumulated data
      digitalWrite(HEART_LED, HIGH);   // turn the HEART_LED on (HIGH is the voltage level)
      memcpy(&nodeData, buf, sizeof(nodeData));
      Console.print("buf size: ");
      Console.println(len);
    }  
    uploadData(nodeData.temp, nodeData.diox, nodeData.cdty);
    digitalWrite(HEART_LED, LOW);    // turn the HEART_LED off by making the voltage LOW
  }
}


void uploadData(float _temp, float _diox, float _cdty) {//Upload Data to ThingSpeak

  // form the string for the API header parameter:
  // form the string for the URL parameter, be careful about the required "
  char stemp[10], sdiox[10], scdty[10], upload_url[150];
  dtostrf(_temp, 1, 2, stemp); 
  dtostrf(_diox, 1, 2, sdiox); 
  dtostrf(_cdty, 1, 2, scdty); 
  sprintf(upload_url, "'https://api.thingspeak.com/update?api_key=A8YN024992BLB75W&field1=%s&field2=%sf&field3=%s'", stemp, sdiox, scdty);
  Console.println("Call Linux Command to Send Data");
  Console.println(upload_url);

  Process p;    // Create a process and call it "p", this process will execute a Linux curl command

//  p.begin("echo");
//  p.addParameter("-k");
//  p.addParameter(upload_url);
//  p.run();    // Run the process and wait for its termination
  Console.print("Feedback from Linux: ");
  p.begin("cat");  // Process that launch the "cat" command
  p.addParameter("/etc/banner");  // Process that launch the "cat" command
  p.run();    // Run the process and wait for its termination
  // If there's output from Linux,
  // send it out the Console:

  while (p.available()) {
    char c = p.read();
    Console.write(c);
  }
  Console.println("");
  Console.println("Call Finished");
  Console.println("####################################");
  Console.println("");
}

//void checkTalkBack() { //Check if there is talkback command
//  char json[200] = {0};
//  int count = 0;
//  char talkBackCommand[20] = {0};
//  char keyWord[] = "SENSORTRIGGER";
//  uint8_t tx_trg[] = "S";
//  uint8_t tlen = sizeof(tx_trg);
//  Console.println("Checking Talkback from Server");  
//  String talkBackURL = "https://api.thingspeak.com/talkbacks/" + talkBackID + "/commands/execute.json?api_key=" + talkBackAPIKey;
//  Console.println(talkBackURL);
//  Process p;    
//  p.begin("curl"); 
//  p.addParameter("-k"); 
//  p.addParameter(talkBackURL);
//  p.run();
//  Console.print("Get Result:");
//
//  while (p.available()) {
//      json[count] = p.read();//
//      Console.write(json[count]);
//      count++;
//  }
//  Console.println("");
//  Console.print("Command Length: ");
//  Console.println(count);
//  
//  if (count > 2) {
//    int quota_count = 0;
//    int start_pos = 0;
//    int stop_pos = 0;
//    for (int i=0;i<count;i++) {
//      if ( json[i] == 34 ) {
//        if( i > 1) {
//          if (json[i-1] == 92) quota_count--; // ignore the quota if it is after \.
//        }
//        quota_count++; // discover quota ";
//      }
//      if (quota_count == 5 && start_pos == 0) start_pos = i;
//      if (quota_count == 6 && stop_pos == 0) {
//        stop_pos = i-1;
//        break;
////      }
//    }
//    Console.print("Quota Count: ");
//    Console.println(quota_count);
//    Console.println("_____________________________________" );
//    Console.print("Get Command String: " );
//    int j=0;
//    for (int i=start_pos+1; i<= stop_pos; i++) {
//      talkBackCommand[j]=json2[i];
//      Console.write(talkBackCommand[j]);
//      j++;
//    }
//    if (strcmp(keyWord,talkBackCommand) == 0) {
//      rf95m.sendto(tx_trg, tlen, RH_BROADCAST_ADDRESS);
//      rf95m.waitPacketSent(); 
//    }
//  }
//}
