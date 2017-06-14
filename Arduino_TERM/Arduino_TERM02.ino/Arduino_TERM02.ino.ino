#include <WiFi.h>
#include<stdlib.h>
#include <Adafruit_GPS.h>      // Using s modified version of the Adafruit libraries
Adafruit_GPS GPS(&Serial1);    // Can't use SoftwareSerial

// WiFi Constants
char ap_ssid[] = "iptime";          // SSID of network
char ap_password[] = "";               // Password of network
unsigned int TimeOut = 10000;           // Milliseconds

/* SENSOR PIN Code */
int PIN_LIGHT = A0;
int PIN_TEMP = A1;
int PIN_Humidity = A2;
int PIN_Trig = 2;
int PIN_Echo = 3;
int buzzor=6;
int led = 13;
// Initialize the client library

// Global Variables
int value = 0;
unsigned int ReportingInterval = 10000;  // How often do you want to send to Ubidots (in millis)
unsigned long LastReport = 0;            // Keep track of when we last sent data
char c;                                  // Used to relay input from the GPS serial feed
String Location = "";                    // Will build the Location string here

// Setup code - runs once
void setup() {
  Serial.begin(19200);       // This is the Serial port that communicates with the Arduino IDR Serial monitor
  GPS.begin(9600);           // Serial1 where we communicate with the Adafruit GPS on Arduino pins 0 and 1 - default baud rate
  Serial.println("Smart Bicycle Assistant");
  pinMode(PIN_Trig,OUTPUT);
  pinMode(PIN_Echo,INPUT);
  pinMode(buzzor,OUTPUT);
  pinMode(led,OUTPUT);
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA); // RMC (recommended minimum) and GGA (fix data) including altitude
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);    // 1 Hz update rate - Adafruit does not suggest using anything higher than 1 Hz
  //GPS.sendCommand(PGCMD_ANTENNA);            // Request updates on antenna status, comment out to keep quiet
  if (WiFi.begin(ap_ssid) != WL_CONNECTED) { // if you are connected, print out info about the connection:
    Serial.println("Couldn't get a wifi connection");
    while(true);
  } 
  Serial.print("Connected to Wifi at IP Address:");
  Serial.println(WiFi.localIP()); 
  GetConnected();  // Leave this here if you want the Edison to stay connected (for testing)
}

// Main loop runs continuosly
void loop() 
{
  while(true){
  long duration, cm;
  
  digitalWrite(PIN_Trig,HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_Trig,LOW);
  duration = pulseIn(PIN_Echo,LOW);
  cm = duration/50;
  Buzz(cm);
  Serial.print(cm);
  Serial.println("cm");
  delay(300);
  if (millis() >= LastReport + ReportingInterval) {
    Serial.print("갱신 시간 : ");
    Serial.print(ReportingInterval/1000);
    Serial.println(" 초");
    
    
    if(GPS.latitude == 0 || GPS.longitude == 0){
    Serial.println("GPS값이 이상하여 다시 측정합니다..");
    Serial.print("GPS값을 제외한 결과를 ");
    }
    Serial.print("서버로 전송:");
    if(GPS.latitude != 0 && GPS.longitude != 0){
    Serial.println(GPS.lastNMEA());   // Use this line for debugging to see raw feed
    Serial.print("Altitude: "); Serial.println(GPS.altitude);
    }
    value = GPS.altitude;    // Altitude as starting point
    if(SendtoServer(String(value))) {
      Serial.println("Data successfully sent to Server!");
      LastReport = millis();  // Reset the timer
      continue;
    }
    else {
      Serial.println("Data not accepted by Server - try again");
    }


  }
       char c = GPS.read();        // Receive output from the GPS one character at a time
  if (GPS.newNMEAreceived()) {  // if a sentence is received, we can check the checksum, parse it...
    
    if (!GPS.parse(GPS.lastNMEA())){   // check to ensure we succeeded with parsing
      
        }                       // If we fail to parse a sentence we just wait for another
  }
  }
}


boolean SendtoServer(String value)
{
  
WiFiClient client;
  char replybuffer[64];          // this is a large buffer for replies
  int count = 0;
  int complete = 0;
  String var = "";
  String le = "";
  ParseLocation();              // Update the location value from the GPS feed
  var="{\"TEMP\":"+ (String)analogRead(PIN_TEMP)+",\"LIGHT\":"+(String)analogRead(PIN_LIGHT)+",\"HUMIDITY\":"+(String)analogRead(PIN_Humidity) + ", \"GPS\":"+ Location + "}";
  if(analogRead(PIN_LIGHT) < 100){
    digitalWrite(led,HIGH);
  }else{
    digitalWrite(led,LOW);
  }
  int num=var.length();                                       // How long is the payload
  le=String(num);                                             //this is to calcule the length of var
  // Make a TCP connection to remote host
  Serial.println("Sending Data to Server");
  if (!client.connect("chlqkrtk2.iptime.org", 3333)) {
    Serial.println("Error: Could not make a TCP connection");
    return false;
  }   
  // Make a HTTP POST request
  client.print("POST / ");
  client.println("HTTP/1.1");
  client.println("Content-Type: application/json");
  client.print("Content-Length: ");
  client.println(le);
  client.println("Host: chlqkrtk2.iptime.org:3333");
  client.println("User-Agent: Arduino/1.0");
  client.println("Connection: close");
  client.println();
  client.println(var);
  client.println();
  unsigned long commandClock = millis();                      // Start the timeout clock
  while(!complete && millis() <= commandClock + TimeOut)         // Need to give the modem time to complete command 
  {
    while(!client.available() &&  millis() <= commandClock + TimeOut);  // Keep checking to see if we have data coming in
    while (client.available()) {
      replybuffer[count] = client.read();
      count++;
      if(count==63) break;
    }
    //Serial.print("count=");  // Uncomment if needed to debug
    //Serial.print(count);
    Serial.print(" - Reply: ");
    for (int i=0; i < count; i++) {
      if (replybuffer[i] != '\n') Serial.write(replybuffer[i]);
    }
    
    return true;
  }
}


// Connection to Wifi 
void GetConnected()
{
  if (WiFi.begin(ap_ssid) != WL_CONNECTED) { // if you are connected, print out info about the connection:
    Serial.println("Couldn't get a wifi connection");
    while(true);
  } 
  Serial.print("Connected to Wifi at IP Address:");
  Serial.println(WiFi.localIP()); 
}

boolean ParseLocation() 
{
  char Latitude[10];
  char Longitude[10];
  float Lat = GPS.latitude;
  float Lon = GPS.longitude;
  float Latcombo = (int)Lat/100 + (((Lat/100 - (int)Lat/100)*100)/60);
  float Loncombo = (int)Lon/100 + (((Lon/100 - (int)Lon/100)*100)/60);
  sprintf(Latitude, "%'.3f", Latcombo);
  sprintf(Longitude, "%'.3f", Loncombo);
  Location = "{\"lat\":" + String(Latitude) + ",\"lng\":" + String(Longitude) + "}";
  if(Lat!= 0 && Lon!=0)
  Serial.println(Location);
}
void Buzz(int cm){  
  if(cm <= 5){
digitalWrite(buzzor,HIGH);
    return;
    
  }
  else if(cm <=15){
  digitalWrite(buzzor,HIGH);
    delay(10);
    digitalWrite(buzzor,LOW);
    return;
    
  }
  else if(cm <=20)
  {
    digitalWrite(buzzor,HIGH);
    delay(10);
    digitalWrite(buzzor,LOW);
    delay(10);
    return;
  }
  else {
    digitalWrite(buzzor,LOW);
    return;
  }
}
