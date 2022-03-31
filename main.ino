#include <math.h>

#include <Wire.h>
#include "MAX30100_PulseOximeter.h"

//////////////////////////Wifi//////////////////////////////////////
#include <WiFi.h>
#include <Firebase_ESP_Client.h>

//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

#define WIFI_SSID "POCOPHONE F1"
#define WIFI_PASSWORD "1234567890"
////////////////////////////////////////////////////////////////////

/////////////////////////////Firebase///////////////////////////////////////////
// Insert Firebase project API Key
#define API_KEY "AIzaSyD50cT994tyyc4B_OS9QIR9qTolE3ZsRzU"

// Insert RTDB URLefine the RTDB URL */
#define DATABASE_URL "https://healthmonitorsystem-cf28f-default-rtdb.firebaseio.com/" 

void SendReadingsToFirebase();

//Define Firebase Data object
FirebaseData FirebaseData;

FirebaseAuth auth;
FirebaseConfig config;

bool signupOK = false;
/////////////////////////////////////////////////////////////////////////////////

#define REPORTING_PERIOD_MS     1000

#define POST_TO_FIREBASE_PERIOD_MS     6000

// Create a PulseOximeter object
PulseOximeter pox;

uint8_t _spo2;
float _heartRate;

// Time at which the last beat occurred
uint32_t tsLastReport = 0;
uint32_t tsLastSend = 0;

// Callback routine is executed when a pulse is detected
//void onBeatDetected() {
//    Serial.println("Beat!");
//}

void setup() {
  
  Serial.begin(115200);

  //////////////////////////////////////////////////
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

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok");
    signupOK = true;
  }
  else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  //////////////////////////////////////////////////

  Serial.print("Initializing pulse oximeter..");

  // Initialize sensor
  if (!pox.begin()) {
    Serial.println("FAILED");
    for(;;);
  } else {
    Serial.println("SUCCESS");
  }

  // Configure sensor to use 7.6mA for LED drive
  pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);

  // Register a callback routine
  //pox.setOnBeatDetectedCallback(onBeatDetected);
}

void loop() {

  // Read from the sensor
  pox.update();
  
  if (millis() - tsLastReport > REPORTING_PERIOD_MS) {
    
    _heartRate = round(pox.getHeartRate());
    _spo2 = round(pox.getSpO2());
    
    Serial.print("Heart rate:");  
    Serial.print(_heartRate);
    Serial.print("bpm / SpO2:");
    Serial.print(_spo2);
    Serial.println("%");
    
    if( millis() - tsLastSend > POST_TO_FIREBASE_PERIOD_MS)
    {
        SendReadingsToFirebase();
        tsLastSend = millis();
    }
    
    tsLastReport = millis();
  }
}

void SendReadingsToFirebase()
{
  pox.shutdown();
  
  if (Firebase.ready() && signupOK){
    if (Firebase.RTDB.setInt(&FirebaseData, "HEARTRATE", _heartRate)){
        Serial.println("PATH: " + FirebaseData.dataPath());
        Serial.println("TYPE: " + FirebaseData.dataType());
    }
    else {
        Serial.println("FAILED");
        Serial.println("REASON: " + FirebaseData.errorReason());
    }
    
      // Write an Float number on the database path test/float
    if (Firebase.RTDB.setInt(&FirebaseData, "SPO2", _spo2)){
        Serial.println("PATH: " + FirebaseData.dataPath());
        Serial.println("TYPE: " + FirebaseData.dataType());
    }
    else 
    {
      Serial.println("FAILED");
      Serial.println("REASON: " + FirebaseData.errorReason());
    }
  }
  pox.resume();
}
