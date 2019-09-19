#include <SPI.h>
#include <WiFi101.h>

/*
GET FEATHER MAC ADDRESS
Jeff Thompson | 2017 | jeffreythompson.org
Extracts the MAC address from an Adafruit Feather 
M0 Wifi board.
NOTE!
Due to the way Arduino's Serial print commands work with
hex values, the MAC address may not include leading zeroes
on some values. If you get a single digit (ie "F"), just
add a zero in front (ie "0F").
 */


void setup() {
  WiFi.setPins(8, 7, 4, 2);

  Serial.begin(9600);
 

  printMacAddress();
}

void loop() {
  printMacAddress();
 
}


void printMacAddress() {
  byte mac[6];
  WiFi.macAddress(mac);
  
  Serial.print(mac[5], HEX);
  Serial.print(":");
  Serial.print(mac[4], HEX);
  Serial.print(":");
  Serial.print(mac[3], HEX);
  Serial.print(":");
  Serial.print(mac[2], HEX);
  Serial.print(":");
  Serial.print(mac[1], HEX);
  Serial.print(":");
  Serial.println(mac[0], HEX);
}
