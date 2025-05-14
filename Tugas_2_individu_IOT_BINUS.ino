#include <WiFi.h>
#include <HTTPClient.h>
#include "DHT.h"

// WiFi Credentials
const char* ssid = "Understriker";
const char* password = "tdfn1103";

// Telegram Bot
String botToken = "8051442806:AAELFL14MwKEyl3ukkqZlM43_72Npub4vmE";
String chatID = "1232762320"; 

// DHT Setup
#define DHTPIN 25
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// LED Pin
#define LEDPIN 12
bool ledTriggered = false;

// Waktu pengiriman berkala
unsigned long lastSendTime = 0;
unsigned long sendInterval = 60000; 

// Waktu polling Telegram
unsigned long lastPollTime = 0;
unsigned long pollInterval = 5000; 
int lastUpdateID = 0;

void setup() {
  Serial.begin(115200);
  dht.begin();
  pinMode(LEDPIN, OUTPUT);
  digitalWrite(LEDPIN, LOW);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected.");
}

void loop() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  // Jika suhu > 25°C dan belum menyala, nyalakan LED & kirim pesan
  if (t > 25.00 && !ledTriggered) {
    digitalWrite(LEDPIN, HIGH);
    ledTriggered = true;
    sendTelegramMessage("🔥 Suhu tinggi terdeteksi!\nSuhu: " + String(t) + "°C\nKelembapan: " + String(h) + "%");
  } else if(h < 50.00 && !ledTriggered) {
    digitalWrite(LEDPIN, HIGH);
    ledTriggered = true;
    sendTelegramMessage("Kelembaban terlalu rendah \nKelembaban: " + String(h) + "%\nSuhu: " + String(t) + "°C");
  }

  // Kirim data berkala
  if (millis() - lastSendTime > sendInterval) {
    lastSendTime = millis();
    String message = "🌡️ Data Sensor:\nSuhu: " + String(t) + "°C\nKelembapan: " + String(h) + "%";
    message += "\nLED: ";
    message += (digitalRead(LEDPIN) == HIGH ? "On" : "Off");
    sendTelegramMessage(message);
  }

  // Cek command dari Telegram
  if (millis() - lastPollTime > pollInterval) {
    lastPollTime = millis();
    checkTelegramCommands();
  }
}

void sendTelegramMessage(String message) {
  if (WiFi.status() == WL_CONNECTED) {
    
    HTTPClient http;
    String url = "https://api.telegram.org/bot" + botToken + "/sendMessage?chat_id=" + chatID + "&text=" + urlencode(message);
    http.begin(url);
    int httpCode = http.GET();
    if (httpCode > 0) {
      Serial.println("Telegram message sent.");
    } else {
      Serial.println("Error sending message.");
    }
    http.end();
  }
}

void checkTelegramCommands() {
  HTTPClient http;
  String url = "https://api.telegram.org/bot" + botToken + "/getUpdates?offset=" + String(lastUpdateID + 1);
  http.begin(url);
  int httpCode = http.GET();

  if (httpCode == 200) {
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    String payload = http.getString();
    Serial.println("Polling Telegram...");
    Serial.println(payload);

    int idx = payload.indexOf("\"update_id\":");
    while (idx != -1) {
      int idStart = idx + 12;
      int idEnd = payload.indexOf(",", idStart);
      lastUpdateID = payload.substring(idStart, idEnd).toInt();

      int msgStart = payload.indexOf("\"text\":\"", idEnd);
      if (msgStart != -1) {
        int msgEnd = payload.indexOf("\"", msgStart + 8);
        String command = payload.substring(msgStart + 8, msgEnd);
        Serial.println("Command: " + command);

        if (command == "/matikan") {
          digitalWrite(LEDPIN, LOW);
          ledTriggered = false;
          sendTelegramMessage("✅ LED telah dimatikan melalui Telegram.");
        }
        else if (command == "/nyalakan") {
          digitalWrite(LEDPIN, HIGH);
          ledTriggered = true;
          sendTelegramMessage("💡 LED telah dinyalakan paksa melalui Telegram.");
        }
        else if(command == "/status") {
          String message = "🌡️ Data Sensor dari command:\nSuhu: " + String(t) + "°C\nKelembapan: " + String(h) + "%";
            message += "\n💡LED: ";
            message += (digitalRead(LEDPIN) == HIGH ? "On" : "Off");
            sendTelegramMessage(message);
        }
      }

      // Cek perintah selanjutnya
      idx = payload.indexOf("\"update_id\":", idEnd);
    }
  }

  http.end();
}

String urlencode(String str) {
  String encoded = "";
  char c;
  char code0, code1;
  for (int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (isalnum(c)) {
      encoded += c;
    } else {
      code1 = (c & 0xf) + '0';
      if ((c & 0xf) > 9) code1 = (c & 0xf) - 10 + 'A';
      c = (c >> 4) & 0xf;
      code0 = c + '0';
      if (c > 9) code0 = c - 10 + 'A';
      encoded += '%';
      encoded += code0;
      encoded += code1;
    }
  }
  return encoded;
}
