#include <ArduinoOTA.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "fauxmoESP.h"
#include <AccelStepper.h>
 
fauxmoESP fauxmo;

#define WIFI_SSID "name"
#define WIFI_PASS "pass"

 
#define SERIAL_BAUDRATE     115200
 
//Define stepper motor connections and sensors
#define dirPin 4 // direction pin
#define stepPin 14 // steps pin
#define enbPin 16 //enable pin
#define IR_Recv 5 
const int sensor1 =12; // set the sensor1 pin
const int sensor2 =13; // set the sensor2 pin

bool state= true;
bool state1= true;

//------------------------------------------------------//
IRrecv irrecv(IR_Recv);
decode_results results;
//-----------------------------------------------------//
 
#define ID_clockwise "clockwise"
#define ID_anticlockwise "anti clockwise"
 
 
char motorDirection = 'S';
 
//Create stepper object
AccelStepper stepper(1,stepPin,dirPin); //motor interface type must be set to 1 when using a driver.

//------------------------------------------------------//
// Wifi
//------------------------------------------------------//

void wifiSetup() {
 
    // Set WIFI module to STA mode
    WiFi.mode(WIFI_STA);
 
    // Connect
    Serial.printf("[WIFI] Connecting to %s ", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
 
    // Wait
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(100);
    }
    Serial.println();
 
    // Connected!
    Serial.printf("[WIFI] STATION Mode, SSID: %s, IP address: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
 
}
 
void setup() {

  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);
  pinMode(enbPin, OUTPUT);
  pinMode(sensor1, INPUT_PULLUP);
  pinMode(sensor2, INPUT_PULLUP);
  digitalWrite(enbPin, HIGH); // put driver on sleep mode
  irrecv.enableIRIn(); // Starts the receiver
 
    // Init serial port and clean garbage
    Serial.begin(SERIAL_BAUDRATE);
    Serial.println();
    Serial.println();
    stepper.setMaxSpeed(5000); //maximum steps per second
 
    // Wifi
    wifiSetup();
 
    // By default, fauxmoESP creates it's own webserver on the defined port
    // The TCP port must be 80 for gen3 devices (default is 1901)
    // This has to be done before the call to enable()
    fauxmo.createServer(true); // not needed, this is the default value
    fauxmo.setPort(80); // This is required for gen3 devices
 
    // You have to call enable(true) once you have a WiFi connection
    // You can enable or disable the library at any moment
    // Disabling it will prevent the devices from being discovered and switched
    fauxmo.enable(true);
 
    // You can use different ways to invoke alexa to modify the devices state:
    // "Alexa, turn clockwise on"
    // "Alexa, turn on anti clockwise"
 
    // Add virtual devices
    fauxmo.addDevice(ID_clockwise );
    fauxmo.addDevice(ID_anticlockwise);

    fauxmo.onSetState([](unsigned char device_id, const char * device_name, bool state, unsigned char value) {
       
        Serial.printf("[MAIN] Device #%d (%s) state: %s value: %d\n", device_id, device_name, state ? "ON" : "OFF", value);
 
        if (strcmp(device_name, ID_clockwise)==0) {
          digitalWrite(enbPin, LOW);
          motorDirection = 'F';
        } else if (strcmp(device_name, ID_anticlockwise)==0) {
          digitalWrite(enbPin, LOW);
          motorDirection = 'R';
        }
 
    });
    
  //OTA Setup
  ArduinoOTA.setHostname("ESP8266-Curtain");
  ArduinoOTA.setPassword("mycurtains");
    
  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {

    ArduinoOTA.handle();
    fauxmo.handle();
    //--------------------------------------------------------//
    state = digitalRead(sensor1);
 
    if (state == LOW){
      digitalWrite(enbPin, HIGH);
      fauxmo.setState(ID_clockwise, false, 0);
         }
   
    state1 = digitalRead(sensor2);
 
    if (state1 == LOW){
      digitalWrite(enbPin, HIGH);
      fauxmo.setState(ID_anticlockwise, false, 0);
        }
 
    //decodes the infrared input
    if (irrecv.decode(&results)){
    long int decCode = results.value;
    Serial.println(decCode);
    //switch case to use the selected remote control button
    switch (results.value){
      case 790335: //when you press the 1 button
      Serial.println("clockwise");
      {
        digitalWrite(enbPin, LOW);
        motorDirection = 'F';
      }
      break;  
      case 876330: //when you press the 2 button
      Serial.println("counterclockwise");
      {
        digitalWrite(enbPin, LOW);
        motorDirection = 'R';
      }
      break;
      case 868140: //when you press the 3 button
      Serial.println("stop");
      {
        digitalWrite(enbPin, HIGH);
        motorDirection = 'S';
      }
      break;
  }
    irrecv.resume(); // Receives the next value from the button you press
  }  
    // This is a sample code to output free heap every 5 seconds
    // This is a cheap way to detect memory leaks
    static unsigned long last = millis();
    if (millis() - last > 5000) {
        last = millis();
        Serial.printf("[MAIN] Free heap: %d bytes\n", ESP.getFreeHeap());
    }
 
    // If your device state is changed by any other means (MQTT, physical button,...)
    // fauxmo.setState(ID_YELLOW, true, 255);

    clockwise();
    anti_clockwise();
}
void clockwise()
{
  if (motorDirection != 'F') {
       return;  // i.e. do nothing
  }
  stepper.setSpeed(3000); //steps per second
  stepper.runSpeed(); //step the motor with constant speed as set by setSpeed()
  ESP.wdtFeed(); // Reset the watch dog timer in case of long rotations.
  }
 
void anti_clockwise()
{
  if (motorDirection != 'R') {
       return;  // i.e. do nothing
  }
  stepper.setSpeed(-3000); //steps per second
  stepper.runSpeed(); //step the motor with constant speed as set by setSpeed()
  ESP.wdtFeed(); // Reset the watch dog timer in case of long rotations.
  }
