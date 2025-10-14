// Sketch: NeoPixel color state changes: red (startup) -> green (WiFi connected) -> blue (fetch success)

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Adafruit_NeoPixel.h>


const char* ssid = "Wokwi-GUEST";
const char* password = "";


#define ENDPOINT_URL "https://td890c551a6f241a653f.free.beeceptor.com/data"


#define NEOPIXEL_PIN 5     
#define NUMPIXELS 1

Adafruit_NeoPixel strip(NUMPIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

void setPixelColorRGB(uint8_t r, uint8_t g, uint8_t b) {
  strip.setPixelColor(0, strip.Color(r, g, b));
  strip.show();
}


void colorWipe(uint32_t color, int wait) {
  for (int i = 0; i < NUMPIXELS; i++) {
    strip.setPixelColor(i, color);
    strip.show();
    delay(wait);
  }
}

void setup() {
  Serial.begin(115200);
  delay(50);
  Serial.println("NeoPixel status demo starting...");

  // Init NeoPixel
  strip.begin();
  strip.show(); 

  // 1) Startup: show RED immediately
  setPixelColorRGB(255, 0, 0);
  Serial.println("State: STARTUP (red)");
  colorWipe(strip.Color(255, 0, 0), 50); // red wipe

  // WiFi connect (with timeout)
  Serial.print("Connecting to WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  unsigned long start = millis();
  const unsigned long wifiTimeout = 15000; 
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < wifiTimeout) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());

    // 2) On WiFi connect: set GREEN
    setPixelColorRGB(0, 255, 0);
    Serial.println("State: WIFI CONNECTED (green)");
    colorWipe(strip.Color(0, 255, 0), 50); // green wipe
  } else {
    Serial.println("WiFi connect timed out. Staying RED (or change as you like).");
    
    setPixelColorRGB(255, 100, 0); 
  }

 
  WiFiClientSecure *client = new WiFiClientSecure;
  client->setInsecure(); 

  // Try to GET data from endpoint if WiFi connected
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    Serial.println("Attempting GET to fetch data...");
    http.begin(*client, ENDPOINT_URL);
    int code = http.GET();
    if (code > 0) {
      Serial.print("GET code: ");
      Serial.println(code);
      String resp = http.getString();
      Serial.print("GET response: ");
      Serial.println(resp);

      // 3) On successful fetch: set BLUE
      setPixelColorRGB(0, 0, 255);
      Serial.println("State: FETCH SUCCESS (blue)");
      colorWipe(strip.Color(0, 0, 255), 50); // blue wipe
    } else {
      Serial.print("GET failed, error code: ");
      Serial.println(code);
      // optional: set amber/yellow for partial failure
      setPixelColorRGB(255, 200, 0);
      Serial.println("State: FETCH FAILED (amber)");
    }
    http.end();
  }

  delete client;
}

void loop() {
 
  delay(1000);
}
