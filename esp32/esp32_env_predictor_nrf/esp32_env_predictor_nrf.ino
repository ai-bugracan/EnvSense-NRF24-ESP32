#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <SPI.h>
#include <RF24.h>

#define DHTPIN 4
#define DHTTYPE DHT11
#define MQ135PIN 36
#define LDRPIN 39

const char* ssid = "Your_SSID";  // WiFi network name (SSID)
const char* password = "Your_Password"; // WiFi password

// Replace "your_server_ip_address" with your actual server IP (e.g., 192.168.1.100 or your domain name)
const char* serverName = "http://your_server_ip_address/data_receiver.php";         // Sensor data
const char* predictionURL = "http://your_server_ip_address/prediction_fetch.php";   // Label prediction

DHT dht(DHTPIN, DHTTYPE);

RF24 radio(26, 5); // CE, CSN
const byte address[6] = "NODE1";

char predictionLabel[50]; // Prediction label (temiz-gunduz-sicak-nemli)

unsigned long lastSensorSend = 0;
unsigned long lastPredictionSend = 0;

const unsigned long sensorInterval = 10000;      // 10 sec ()
const unsigned long predictionInterval = 600000; // 10 min ()

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\n WiFi connected!");

  dht.begin();

  radio.begin();
  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_LOW);
  radio.stopListening();
}

void sendSensorData() {
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  int mq135 = analogRead(MQ135PIN);
  int ldr = analogRead(LDRPIN);

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("DHT11 data could not be read!");
    return;
  }

  HTTPClient http;
  http.begin(serverName);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  String postData = "temperature=" + String(temperature, 1) +
                    "&humidity=" + String(humidity, 1) +
                    "&mq135=" + String(mq135) +
                    "&ldr=" + String(ldr);

  int httpResponseCode = http.POST(postData);

  if (httpResponseCode > 0) {
    Serial.println("Sensor data transmitted!");
    Serial.println(http.getString());
  } else {
    Serial.print("HTTP error: ");
    Serial.println(httpResponseCode);
  }

  http.end();
}

void fetchAndSendPredictionLabel() {
  HTTPClient http;
  http.begin(predictionURL);
  int httpCode = http.GET();

  if (httpCode == 200) {
    String payload = http.getString(); // e.g.: {"predicted_label":"kirli-gece-normal-normal","prediction_time":"..."}
    Serial.println("Raw data: " + payload);

    // Manually parse the predicted_label value simply
    int startIndex = payload.indexOf("\"predicted_label\":\"");
    if (startIndex != -1) {
      startIndex += strlen("\"predicted_label\":\"");
      int endIndex = payload.indexOf("\"", startIndex);
      if (endIndex != -1) {
        String label = payload.substring(startIndex, endIndex);
        Serial.println("Extracted label: " + label);

        // Copy to predictionLabel array
        label.toCharArray(predictionLabel, sizeof(predictionLabel));

        // Send via NRF (only the label array, size label.length() + 1)
        bool success = radio.write(&predictionLabel, label.length() + 1);
        if (success) {
          Serial.println("Prediction label sent via NRF!");
        } else {
          Serial.println("NRF transmission failed!");
        }

      } else {
        Serial.println("Closing quote for label not found!");
      }
    } else {
      Serial.println("Opening quote for label not found!");
    }

  } else {
    Serial.print("HTTP error: ");
    Serial.println(httpCode);
  }

  http.end();
}

void loop() {
  unsigned long currentMillis = millis();

  if (WiFi.status() == WL_CONNECTED) {
    if (currentMillis - lastSensorSend >= sensorInterval) {
      sendSensorData();
      lastSensorSend = currentMillis;
    }

    if (currentMillis - lastPredictionSend >= predictionInterval) {
      fetchAndSendPredictionLabel();
      lastPredictionSend = currentMillis;
    }
  } else {
    Serial.println("No WIFI connection.");
  }
}
