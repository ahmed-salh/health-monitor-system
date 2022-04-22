#include <math.h>

// Start GPS
#include <TinyGPS++.h>
#include <HardwareSerial.h>

#define GPS_REPORTING_PERIOD_MS     10000

#define RXD2 16
#define TXD2 17

TinyGPSPlus gps;
HardwareSerial gpsSerial(1);

float LAT = 0;
float LONG = 0;
float ALT = 0;

TaskHandle_t GetGPSReadings;

bool locationIsValid = false;
uint32_t  gpsLastReport;
// End GPS

// Start Wifi
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#define WIFI_SSID "samy"
#define WIFI_PASSWORD "s@m9102;++--SAMY31/12/2022"
// End Wifi

// Start Firebase
#include "addons/TokenHelper.h" //Provide the token generation process info.
#include "addons/RTDBHelper.h"  //Provide the RTDB payload printing info and other helper functions.
#define API_KEY "AIzaSyD50cT994tyyc4B_OS9QIR9qTolE3ZsRzU" // Insert Firebase project API Key
#define DATABASE_URL "https://healthmonitorsystem-cf28f-default-rtdb.firebaseio.com/" // Insert RTDB URLefine the RTDB URL */

FirebaseData FirebaseData;  //Firebase data object
FirebaseAuth auth;  //Firebase authentication object
FirebaseConfig config;  //Firebase configuration object

TaskHandle_t PostToFirebase;
bool signupOK = false;
// End Firebase

// Start Function Declaration
void SignUpToFirebase();
void InitializeWifi();
void InitializePOX();
void InitializeGPS();

void SendReadingsToFirebase();
void SensorReadings();
void GetLocalGPS();
// End Function Declaration

// Start Pulse Oximeter
#include <Wire.h>
#include "MAX30100_PulseOximeter.h"
#define POX_REPORTING_PERIOD_MS  1000

PulseOximeter pox;  // Create a PulseOximeter object

TaskHandle_t GetReadings;

uint8_t _spo2;
uint8_t _heartRate;

uint32_t poxLastReport = 0;
// End Pulse Oximeter

void setup() {
  
  Serial.begin(115200); //Begin serial communication

  InitializeWifi();

  SignUpToFirebase();

  InitializeGPS();
  
  InitializePOX();

  xTaskCreatePinnedToCore(SensorReadings, "GetReadings", 1724, NULL, 0, &GetReadings, 0);

  xTaskCreatePinnedToCore(GetLocalGPS, "GetGPSReadings", 1124, NULL, 0, &GetGPSReadings, 0);
  
  xTaskCreatePinnedToCore(SendReadingsToFirebase, "PostToFirebase", 6500, NULL, 0, &PostToFirebase, 1);
}

void SensorReadings(void * parameter)
{
  for(;;)
  {
    // Read from the sensor
    pox.update();
      
    if (millis() - poxLastReport > POX_REPORTING_PERIOD_MS) {
      _heartRate = round(pox.getHeartRate());
      _spo2 = round(pox.getSpO2());
    
      Serial.print("Heart rate:");  
      Serial.print(_heartRate);
      Serial.print("bpm / SpO2:");
      Serial.print(_spo2);
      Serial.println("%");
    
      poxLastReport = millis();
    }
  }
}

void SendReadingsToFirebase(void * parameter)
{
  for(;;)
  {
    if (Firebase.ready() && signupOK){
      
      if (Firebase.RTDB.setInt(&FirebaseData, "HEARTRATE", _heartRate)){
          Serial.println("PATH: " + FirebaseData.dataPath());
          Serial.println("TYPE: " + FirebaseData.dataType());
      }
    
      if (Firebase.RTDB.setInt(&FirebaseData, "SPO2", _spo2)){
          Serial.println("PATH: " + FirebaseData.dataPath());
          Serial.println("TYPE: " + FirebaseData.dataType());
      }
      
      if(locationIsValid){
          if (Firebase.RTDB.setFloat(&FirebaseData, "LATITUDE", LAT)){
              Serial.println("PATH: " + FirebaseData.dataPath());
              Serial.println("TYPE: " + FirebaseData.dataType());
          }
      
          if (Firebase.RTDB.setFloat(&FirebaseData, "LONGITUDE", LONG)){
              Serial.println("PATH: " + FirebaseData.dataPath());
              Serial.println("TYPE: " + FirebaseData.dataType());
          }

          if (Firebase.RTDB.setFloat(&FirebaseData, "ALTITUDE", ALT)){
              Serial.println("PATH: " + FirebaseData.dataPath());
              Serial.println("TYPE: " + FirebaseData.dataType());
          }
      }
    }
  }
}

void InitializeWifi()
{
  // Wifi Initialize ...
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(300);
  }
  
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
}

void SignUpToFirebase()
{
  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", ""))
  {
    Serial.println("ok");
    signupOK = true;
  }
  else
  {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void InitializePOX()
{
  Serial.print("Initializing pulse oximeter..");

  // Initialize sensor
  if (!pox.begin()) {
    Serial.println("FAILED");
    for(;;);
  } else {
    Serial.println("SUCCESS");
  }

  // Configure sensor to use 7.6mA for LED drive
  //pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);
}

void GetLocalGPS(void * parameter)
{
  for(;;){
    
    smartdelay(GPS_REPORTING_PERIOD_MS);

    if(gps.location.isValid())
    {
      locationIsValid = true;
          
      LAT = gps.location.lat();
      LONG = gps.location.lng();
      ALT = gps.altitude.meters();
    }
    else
    {
      locationIsValid = false;
    }
  }
}

void InitializeGPS()
{
  //Begin serial communication Neo6mGPS
  gpsSerial.begin(9600, SERIAL_8N1, RXD2, TXD2);
}

static void smartdelay(unsigned long ms)
{
  unsigned long start = millis();
  do 
  {
    while (gpsSerial.available())
      gps.encode(gpsSerial.read());
  } while (millis() - start < ms);
}

void loop()
{
  delay(1);  
}
