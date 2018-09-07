#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <DHTNew.h>

// For SSID(MYSSID) name and password(PASSWORD)
#include "secrets.h"

// ---- Configuration ----
#define DEBUG false
// Only one of these should be true, HTML has higher priority than JSON.
#define HTML false
#define JSON true

// ---- Definitions ----
#define DHTTYPE DHT_MODEL_DHT11
#define PORT 80
#define BUFFERSIZE 7

// ---- WiFi Configuration ----
const char* ssid = MYSSID;
const char* password = PASSWORD;
WiFiServer server(PORT);

// ---- DHT Configuration ----
const int DHTPin = 2;
DHT dht(DHTPin, DHTTYPE);

// ---- Global Buffers ----
static char celsiusBuffer[BUFFERSIZE];
static char humidityBuffer[BUFFERSIZE];

void deliver_html(WiFiClient *);
void deliver_json(WiFiClient *);

void setup() {
  #if DEBUG
    Serial.begin(9600);
  #endif

  delay(10);

  dht.begin();

  #if DEBUG
    Serial.println();
    Serial.print("Connecting to: ");
    Serial.println(ssid);
  #endif

  WiFi.begin(ssid, password);

  #if DEBUG
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println();
    Serial.println("WiFi connected.");
  #endif

  server.begin();

  #if DEBUG
    Serial.println("Web server running. Waiting for IP...");
  #endif

  delay(10000);

  #if DEBUG
    Serial.println(WiFi.localIP());
  #endif
}

void loop() {
  WiFiClient client = server.available();

  if (client) {
    #if DEBUG
      Serial.println("New client.");
    #endif

    bool blank_line = true;

    while (client.connected()) {
      if (client.available()) {
        char c = client.read();

        if (c == '\n' && blank_line) {
          float humidity = dht.readHumidity();
          float temperature = dht.readTemperature();

          if (isnan(humidity) || isnan(temperature)) {
            #if DEBUG
              Serial.println("Failed to read from DHT sensor.");
            #endif

            strcpy(celsiusBuffer, "failed");
            strcpy(humidityBuffer, "failed");
          } else {
            dtostrf(temperature, 6, 2, celsiusBuffer);
            dtostrf(humidity, 6, 2, humidityBuffer);

            #if DEBUG
              Serial.print("Humidity: ");
              Serial.println(humidity);
              Serial.print("Temperature: ");
              Serial.println(temperature);
            #endif
          }

          #if HTML
            deliver_html(&client);
          #else if JSON
            deliver_json(&client);
          #endif

          break;
        }

        if (c == '\n') {
          blank_line = true;
        } else if (c != '\r') {
          blank_line = false;
        }
      }
    }
    delay(1);
    client.stop();

    #if DEBUG
      Serial.println("Client disconnected.");
    #endif
  }
}

void deliver_html(WiFiClient *client) {
  client->println("HTTP/1.1 200 OK");
  client->println("Content-Type: text/html");
  client->println("Connection: close");
  client->println();
  client->println("<!DOCTYPE HTML>");
  client->println("<html>");
  client->println("<head></head><body>");
  client->println("<h1>ESP8266 - Temperature and Humidity</h1>");
  client->print("<h3>Temperature: ");
  client->print(celsiusBuffer);
  client->println("&#176;C</h3>");
  client->print("<h3>Humidity: ");
  client->print(humidityBuffer);
  client->println("%</h3>");
  client->println("</body></html>");
}

void deliver_json(WiFiClient *client) {
  client->println("HTTP/1.1 200 OK");
  client->println("Content-type: application/json");
  client->println("Connection: close");
  client->println();
  client->println("{");
  client->print("\"temperature\":");
  client->print(celsiusBuffer);
  client->println(",");
  client->print("\"humidity\":");
  client->println(humidityBuffer);
  client->println("}");
}
