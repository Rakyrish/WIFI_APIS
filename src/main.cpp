#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Adafruit_NeoPixel.h>
#include <Keypad.h>
#include <ArduinoJson.h>

const char* ssid = "Wokwi-GUEST";
const char* password = "";


const char* STK_URL = "https://mbugua.pythonanywhere.com/stkpush";
const char* STATUS_URL = "https://pytestapi.pythonanywhere.com/api/device/yFGXRRpPsYD9/transactions/";


#define NEOPIXEL_PIN 5
#define NUMPIXELS 1
Adafruit_NeoPixel strip(NUMPIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);


#define SIGNAL_PIN 4

const byte ROWS = 4;
const byte COLS = 4;

char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

byte rowPins[ROWS] = {13, 12, 14, 27};
byte colPins[COLS] = {26, 25, 33, 32};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);



// const byte ROWS = 4;
// const byte COLS = 3;
// char keys[ROWS][COLS] = {
//   {'1','2','3'},
//   {'4','5','6'},
//   {'7','8','9'},
//   {'*','0','#'}
// };
// byte rowPins[ROWS] = {13, 12, 14, 27};
// byte colPins[COLS] = {26, 25, 33};
// Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);


String phoneNumber = "";
const int defaultAmount = 1;

enum TransactionState { TX_IDLE, TX_WAITING_PAYMENT, TX_SUCCESS };
TransactionState txState = TX_IDLE;

unsigned long txStartTime = 0;
unsigned long lastBlink = 0;
unsigned long lastCheck = 0;

const int BLINK_INTERVAL = 400; // ms
const unsigned long CHECK_INTERVAL = 5000; 
const unsigned long SUCCESS_DURATION = 15000; 


void setColor(uint8_t r, uint8_t g, uint8_t b) {
  strip.setPixelColor(0, strip.Color(r, g, b));
  strip.show();
}


void updateBlinkingLED() {
  if (txState == TX_WAITING_PAYMENT) {
    if (millis() - lastBlink >= BLINK_INTERVAL) {
      lastBlink = millis();
      static bool blinkState = false;
      blinkState = !blinkState;
      if (blinkState)
        setColor(0, 255, 0); 
      else
        setColor(0, 0, 255); 
    }
  }
}

// --- Connect to WiFi ---
void connectWiFi() {
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println("\nWiFi Connected! IP: " + WiFi.localIP().toString());
  Serial.println("input the phone number followed by # to initiate payment");
  setColor(0, 0, 255); 
}


bool sendSTKPush(String phone) {
  if (WiFi.status() != WL_CONNECTED) connectWiFi();

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient https;

  https.begin(client, STK_URL);
  https.addHeader("Content-Type", "application/json");

  String json = "{\"phone\":\"" + phone + "\",\"amount\":" + String(defaultAmount) + "}";
  Serial.println("Requesting STK Push for: " + phone + " Amount: " + String(defaultAmount));

  int httpCode = https.POST(json);
  if (httpCode > 0) {
    String response = https.getString();
    Serial.println("STK Response: " + response);
    https.end();
    return true;
  } else {
    Serial.println("❌ STK Push failed!");
    https.end();
    return false;
  }
}


String checkPaymentStatus() {
  if (WiFi.status() != WL_CONNECTED) connectWiFi();

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient https;

  https.begin(client, STATUS_URL);
  int httpCode = https.GET();

  if (httpCode > 0) {
    String response = https.getString();
    Serial.println("Status Response: " + response);

    DynamicJsonDocument doc(1024);
    deserializeJson(doc, response);

   
    for (JsonObject tx : doc.as<JsonArray>()) {
      int resultCode = tx["ResultCode"];
      if (resultCode == 0) return "success";
    }
    return "fail";
  } else {
    Serial.println("❌ Failed to get status");
    return "error";
  }
}


void setup() {
  Serial.begin(115200);
  pinMode(SIGNAL_PIN, OUTPUT);
  digitalWrite(SIGNAL_PIN, LOW);

  strip.begin();
  strip.show();

  setColor(255, 0, 0); 
  delay(1000);

  connectWiFi();
}


void loop() {
  char key = keypad.getKey();

  
  if (txState == TX_IDLE && key) {
    if (key >= '0' && key <= '9') {
      phoneNumber += key;
      Serial.println(key);
    }
    else if (key == '*') {
      phoneNumber = "";
      Serial.println("🧹 Cleared phone input");
    }
    else if (key == '#') {
      if (phoneNumber.length() == 10 && phoneNumber.startsWith("0")) {
        String intlPhone = "254" + phoneNumber.substring(1);
        Serial.println("📱 Phone number : " + intlPhone);

        if (sendSTKPush(intlPhone)) {
          txState = TX_WAITING_PAYMENT;
          txStartTime = millis();
          lastCheck = 0; 
        }
      } else {
        Serial.println("⚠️ Invalid phone number!");
        phoneNumber = "";
      }
    }
  }

  
  updateBlinkingLED();

 
  if (txState == TX_WAITING_PAYMENT && millis() - lastCheck >= CHECK_INTERVAL) {
    lastCheck = millis();
    String status = checkPaymentStatus();

    if (status == "success") {
      Serial.println("✅ Payment confirmed!");
      setColor(0, 255, 0);      // Solid Green
      digitalWrite(SIGNAL_PIN, HIGH);
      txStartTime = millis();
      txState = TX_SUCCESS;
    } else if (status == "fail" || status == "error") {
      Serial.println("❌ Payment failed.");
      setColor(0, 0, 255);      // Blue
      digitalWrite(SIGNAL_PIN, LOW);
      txState = TX_IDLE;
      phoneNumber = "";
    }
  }

 
  if (txState == TX_SUCCESS && millis() - txStartTime >= SUCCESS_DURATION) {
    txState = TX_IDLE;
    setColor(0, 0, 255);      // Back to Blue
    digitalWrite(SIGNAL_PIN, LOW);
    phoneNumber = "";
  }
}









// #include <Arduino.h>
// #include <WiFi.h>
// #include <WiFiClientSecure.h>
// #include <HTTPClient.h>
// #include <ArduinoJson.h>
// #include <Keypad.h>
// #include <Adafruit_NeoPixel.h>
// #include <base64.h>
// #include <time.h>

// #include "secrets.h"

// // ===== WiFi =====
// const char* ssid = "Wokwi-GUEST";
// const char* password = "";

// // ===== LED & Signal Pin =====
// #define NEOPIXEL_PIN 5
// #define NUMPIXELS 1
// #define SIGNAL_PIN 4
// Adafruit_NeoPixel strip(NUMPIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

// // ===== Keypad =====
// const byte ROWS = 4, COLS = 3;
// char keys[ROWS][COLS] = {
//   {'1','2','3'},
//   {'4','5','6'},
//   {'7','8','9'},
//   {'*','0','#'}
// };
// byte rowPins[ROWS] = {13, 12, 14, 27};
// byte colPins[COLS] = {26, 25, 33};
// Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// String phoneNumber = "";
// const int defaultAmount = 1;

// // ===== Transaction States =====
// enum TransactionState { TX_IDLE, TX_WAITING_PAYMENT, TX_SUCCESS };
// TransactionState txState = TX_IDLE;

// unsigned long txStartTime = 0;
// unsigned long lastBlink = 0;
// unsigned long lastCheck = 0;

// const int BLINK_INTERVAL = 400; // ms
// const unsigned long SUCCESS_DURATION = 15000; 
// const unsigned long CHECK_INTERVAL = 5000; 

// // ===== Helpers =====
// void setColor(uint8_t r, uint8_t g, uint8_t b) {
//   strip.setPixelColor(0, strip.Color(r, g, b));
//   strip.show();
// }

// void updateBlinkingLED() {
//   if (txState == TX_WAITING_PAYMENT) {
//     if (millis() - lastBlink >= BLINK_INTERVAL) {
//       lastBlink = millis();
//       static bool blinkState = false;
//       blinkState = !blinkState;
//       if (blinkState)
//         setColor(0, 255, 0); // Green blink
//       else
//         setColor(0, 0, 255); // Blue blink
//     }
//   }
// }

// // ===== WiFi & Time =====
// void connectWiFi() {
//   Serial.print("Connecting to WiFi...");
//   WiFi.begin(ssid, password);
//   while (WiFi.status() != WL_CONNECTED) delay(500);
//   Serial.println("\nWiFi Connected. IP: " + WiFi.localIP().toString());
//   setColor(0, 0, 255);
// }

// const char* ntpServer = "pool.ntp.org";

// void setupTime() {
//   configTime(0, 0, ntpServer);
//   Serial.print("Waiting for NTP time");
//   time_t now = time(NULL);
//   while (now < 100000) {
//     Serial.print(".");
//     delay(500);
//     now = time(NULL);
//   }
//   Serial.println("\nTime set: " + String(ctime(&now)));
// }


// // ===== TLS Test =====
// bool testTLS() {
//   WiFiClientSecure client;
//   client.setInsecure(); // allow TLS without cert
//   Serial.println("Testing TLS connection to sandbox.safaricom.co.ke...");
//   if (!client.connect("sandbox.safaricom.co.ke", 443)) {
//     Serial.println("❌ TLS connection failed!");
//     return false;
//   }
//   Serial.println("✅ TLS connection succeeded!");
//   client.stop();
//   return true;
// }

// // ===== Access Token =====
// String getAccessToken() {
//   String auth = String(CONSUMER_KEY) + ":" + String(CONSUMER_SECRET);
//   String encodedAuth = base64::encode(auth);

//   WiFiClientSecure client;
//   client.setInsecure();
//   HTTPClient https;

//   const char* OAUTH_URL = "https://sandbox.safaricom.co.ke/oauth/v1/generate?grant_type=client_credentials";
//   https.begin(client, OAUTH_URL);
//   https.addHeader("Authorization", "Basic " + encodedAuth);

//   int code = https.GET();
//   if (code > 0) {
//     String body = https.getString();
//     int start = body.indexOf("access_token\":\"") + 15;
//     int end = body.indexOf("\"", start);
//     https.end();
//     return body.substring(start, end);
//   }
//   Serial.println("Token Error: " + String(code));
//   https.end();
//   return "";
// }

// // ===== STK Push =====
// bool sendSTKPush(String phone) {
//   String token = getAccessToken();
//   if (token == "") {
//     Serial.println("❌ Failed to get access token");
//     return false;
//   }

//   // Timestamp
//   time_t now = time(NULL);
//   struct tm* t = localtime(&now);
//   char timestamp[20];
//   sprintf(timestamp, "%04d%02d%02d%02d%02d%02d",
//           t->tm_year+1900, t->tm_mon+1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);

//   String password = base64::encode(String(SHORTCODE) + PASSKEY + String(timestamp));

//   WiFiClientSecure client;
//   client.setInsecure();
//   HTTPClient https;

//   const char* SAF_STK_URL = "https://sandbox.safaricom.co.ke/mpesa/stkpush/v1/processrequest";
//   https.begin(client, SAF_STK_URL);
//   https.addHeader("Content-Type", "application/json");
//   https.addHeader("Authorization", "Bearer " + token);

//   String json = "{"
//     "\"BusinessShortCode\":\"" + String(SHORTCODE) + "\","
//     "\"Password\":\"" + password + "\","
//     "\"Timestamp\":\"" + String(timestamp) + "\","
//     "\"TransactionType\":\"CustomerPayBillOnline\","
//     "\"Amount\":\"" + String(defaultAmount) + "\","
//     "\"PartyA\":\"" + phone + "\","
//     "\"PartyB\":\"" + String(SHORTCODE) + "\","
//     "\"PhoneNumber\":\"" + phone + "\","
//     "\"CallBackURL\":\"" + String(CALLBACK_URL) + "\","
//     "\"AccountReference\":\"Test\","
//     "\"TransactionDesc\":\"Payment\""
//   "}";

//   int code = https.POST(json);
//   if (code > 0) {
//     Serial.println("STK Response: " + https.getString());
//     https.end();
//     return true;
//   }

//   Serial.println("❌ STK push failed: " + String(code));
//   https.end();
//   return false;
// }

// // ===== Setup =====
// void setup() {
//   Serial.begin(115200);
//   strip.begin(); strip.show();
//   pinMode(SIGNAL_PIN, OUTPUT); digitalWrite(SIGNAL_PIN, LOW);

//   connectWiFi();
//   delay(1000);      // give WiFi time
//   setupTime();

//   if (!testTLS()) {
//     setColor(255, 0, 0); // Red if TLS fails
//     while(true) delay(1000); // stop further execution
//   }

//   setColor(0, 0, 255); // Blue idle
// }

// // ===== Loop =====
// void loop() {
//   char key = keypad.getKey();

//   if (txState == TX_IDLE && key) {
//     if (key >= '0' && key <= '9') { phoneNumber += key; Serial.println(phoneNumber); }
//     else if (key == '*') { phoneNumber = ""; Serial.println("Cleared phone input"); }
//     else if (key == '#') {
//       if (phoneNumber.length() == 10 && phoneNumber.startsWith("0")) {
//         String intlPhone = "254" + phoneNumber.substring(1);
//         Serial.println("📱 Sending STK Push to: " + intlPhone);

//         if (sendSTKPush(intlPhone)) {
//           txState = TX_WAITING_PAYMENT; txStartTime = millis(); lastCheck = 0;
//           Serial.println("⏳ Waiting for payment...");
//         } else {
//           Serial.println("❌ STK push failed");
//           setColor(255,0,0); digitalWrite(SIGNAL_PIN, LOW);
//         }
//       } else { Serial.println("⚠️ Invalid phone number!"); }
//       phoneNumber = "";
//     }
//   }

//   updateBlinkingLED();

//   // Payment polling
//   if (txState == TX_WAITING_PAYMENT && millis()-lastCheck >= CHECK_INTERVAL) {
//     lastCheck = millis();
//     String status = "success"; // stub, replace with your status check if available

//     if (status == "success") {
//       Serial.println("✅ Payment confirmed!");
//       setColor(0,255,0); digitalWrite(SIGNAL_PIN,HIGH);
//       txStartTime = millis(); txState = TX_SUCCESS;
//     } else if (status == "fail" || status=="error") {
//       Serial.println("❌ Payment failed"); setColor(255,0,0); digitalWrite(SIGNAL_PIN,LOW);
//       txState = TX_IDLE; phoneNumber="";
//     }
//   }

//   if (txState == TX_SUCCESS && millis()-txStartTime >= SUCCESS_DURATION) {
//     txState = TX_IDLE; setColor(0,0,255); digitalWrite(SIGNAL_PIN,LOW);
//   }
// }
