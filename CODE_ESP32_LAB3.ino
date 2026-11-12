#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <DHT.h>

// =====================================
// 1. Wi-Fi Configuration
// =====================================
const char* ssid = "Camelia iPhone";
const char* password = "Camelia123";

// Google Apps Script Web App URL
// Use the original URL ending in /exec
const char* scriptURL =
  "https://script.google.com/macros/s/AKfycby6_SahOJ63RuQdfQnxLNI3cKU8GW1-Jzlg3EQAdFCNMqP8aBjU614n-Al37KURdCA/exec";

// =====================================
// 2. DHT11 Sensor Configuration
// =====================================
#define DHTPIN 16
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

// =====================================
// 3. Data Sending Interval
// =====================================
// 600000 ms = 10 minutes
const unsigned long sendInterval = 600000;

unsigned long previousMillis = 0;

// =====================================
// 4. Connect to Wi-Fi
// =====================================
void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  Serial.println("Connecting to WiFi...");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  unsigned long startTime = millis();

  while (WiFi.status() != WL_CONNECTED &&
         millis() - startTime < 20000) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi connection failed!");
  }
}

// =====================================
// 5. Send Data to Google Sheets
// =====================================
void sendDataToGoogleSheets(
  float temperature,
  float humidity
) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected!");
    return;
  }

  WiFiClientSecure client;

  // Lab testing only:
  // Skip certificate verification
  client.setInsecure();

  HTTPClient http;
  http.setTimeout(15000);
  http.setFollowRedirects(
    HTTPC_STRICT_FOLLOW_REDIRECTS
  );

  if (!http.begin(client, scriptURL)) {
    Serial.println("HTTP connection failed!");
    return;
  }

  http.addHeader(
    "Content-Type",
    "application/json"
  );

  // Create JSON payload
  String jsonData =
    "{\"temperature\":" +
    String(temperature, 2) +
    ",\"humidity\":" +
    String(humidity, 2) +
    "}";

  Serial.println("Sending data...");
  Serial.println(jsonData);

  String url = String(scriptURL) +
  "?temperature=" + String(temperature, 2) +
  "&humidity=" + String(humidity, 2);

http.end();

if (!http.begin(client, url)) {
  Serial.println("Failed to open URL!");
  return;
}

int httpResponseCode = http.GET();

  if (httpResponseCode > 0) {
    Serial.print("HTTP Response Code: ");
    Serial.println(httpResponseCode);

    String response = http.getString();
    Serial.println("Response:");
    Serial.println(response);
  } else {
    Serial.print("HTTP Error: ");
    Serial.println(
      http.errorToString(httpResponseCode)
    );
  }

  http.end();
}

// =====================================
// 6. Setup
// =====================================
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("======================");
  Serial.println("ESP32 DHT11 Data Logger");
  Serial.println("======================");

  dht.begin();

  connectWiFi();

  // Send first reading immediately
  previousMillis = millis() - sendInterval;
}

// =====================================
// 7. Main Loop
// =====================================
void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >=
      sendInterval) {

    previousMillis = currentMillis;

    // Reconnect Wi-Fi if necessary
    connectWiFi();

    // Read DHT11 sensor
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();

    // Validate sensor readings
    if (isnan(humidity) ||
        isnan(temperature)) {
      Serial.println(
        "Failed to read DHT11 sensor!"
      );
      return;
    }

    // Display sensor readings
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.print(" °C | Humidity: ");
    Serial.print(humidity);
    Serial.println(" %");

    // Send readings to Google Sheets
    sendDataToGoogleSheets(
      temperature,
      humidity
    );
  }
}
  