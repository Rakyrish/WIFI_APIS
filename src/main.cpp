#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>


const char* ssid = "Wokwi-GUEST";
const char* password = "";


const char* DEVICE_ID = ";X`ih!.8~2;5";
const char* BASE_URL = "https://pytestapi.pythonanywhere.com/api/gettransaction/?device_id=";


#define NEOPIXEL_PIN 5
#define SIGNAL_PIN 4   
#define NUMPIXELS 1


Adafruit_NeoPixel strip(NUMPIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);


void setPixelColorRGB(uint8_t r, uint8_t g, uint8_t b) {
  strip.setPixelColor(0, strip.Color(r, g, b));
  strip.show();
}

void blinkBlue(int times, int delayMs) {
  for (int i = 0; i < times; i++) {
    setPixelColorRGB(0, 0, 255); // Blue ON
    delay(delayMs);
    setPixelColorRGB(0, 0, 0);   // OFF
    delay(delayMs);
  }
}


void connectWiFi() {
  Serial.print("Connecting to WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  blinkBlue(3, 500);


  unsigned long start = millis();
  const unsigned long wifiTimeout = 15000; 
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < wifiTimeout) {
    Serial.print(".");
    // delay(500);
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
     setPixelColorRGB(0, 0, 255); //blue
  } else {
    Serial.println("WiFi connection failed!");
    setPixelColorRGB(255, 100, 0); // Orange: Failed
  }
}

void setup() {
  Serial.begin(115200);
  delay(100);

  pinMode(SIGNAL_PIN, OUTPUT);
  digitalWrite(SIGNAL_PIN, LOW);

  strip.begin();
  strip.show();

  // Startup color
  setPixelColorRGB(255, 0, 0); // Red
  Serial.println("State: STARTUP (red)");

  connectWiFi();
}


void loop() {
  static bool signalActive = false;           
  static unsigned long signalEndTime = 0;     
  static unsigned long nextRequestTime = 0;   
  unsigned long currentTime = millis();       

  
  if (signalActive && currentTime >= signalEndTime) {
    digitalWrite(SIGNAL_PIN, LOW);        
    setPixelColorRGB(0, 0, 255);            
    Serial.println("Signal OFF. Waiting 5 minutes before next request...");
    signalActive = false;
    nextRequestTime = currentTime + 300000; 
  }

  
  if (!signalActive && currentTime >= nextRequestTime) {
    if (WiFi.status() == WL_CONNECTED) {
      WiFiClientSecure client;
      client.setInsecure();
      HTTPClient http;

      String fullUrl = String(BASE_URL) + DEVICE_ID;
      Serial.print("Requesting: ");
      Serial.println(fullUrl);

      if (http.begin(client, fullUrl)) {
        int httpCode = http.GET();
        Serial.print("HTTP Response code: ");
        Serial.println(httpCode);

        if (httpCode == HTTP_CODE_OK) {
          String payload = http.getString();
          Serial.println("Response: " + payload);

          StaticJsonDocument<300> doc;
          DeserializationError error = deserializeJson(doc, payload);

          if (!error) {
            const char* transAmount = doc[0]["TransAmount"];
            float amount = atof(transAmount);

            Serial.print("TransAmount: ");
            Serial.println(amount);

            if (amount >= 20.0) {
              Serial.println("✅ Transaction found! Turning signal ON...");
              digitalWrite(SIGNAL_PIN, HIGH);
              setPixelColorRGB(0, 255, 0);    
              signalActive = true;
              signalEndTime = currentTime + 30000;  

              
            } else {
              Serial.println("No valid transaction (amount < 20).");
              nextRequestTime = currentTime + 1000;  
            }
          } else {
            Serial.println("❌ JSON Parse Error!");
            Serial.println(error.c_str());
            nextRequestTime = currentTime + 5000;
          }
        } else {
          Serial.printf("HTTP Error: %d\n", httpCode);
          setPixelColorRGB(255, 200, 0);
          nextRequestTime = currentTime + 10000; 
        }

        http.end();
      } else {
        Serial.println("Failed to begin HTTP client");
        nextRequestTime = currentTime + 10000;
      }
    } else {
      Serial.println("WiFi disconnected! Reconnecting...");
      connectWiFi();
      nextRequestTime = currentTime + 5000;
    }
  }

  delay(100);
}

