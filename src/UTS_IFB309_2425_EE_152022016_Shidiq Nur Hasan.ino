#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// Konfigurasi WiFi
const char* ssid = "Wokwi-GUEST";
const char* pass = "";  // Pastikan ini adalah password WiFi yang benar

// Konfigurasi MQTT
const char* mqtt_server = "broker.hivemq.com";
const char* clientName = "espShidiq";  // Nama Client MQTT
const char* topicSuhu = "Shidiq152022016/suhu";        
const char* topicKelembaban = "Shidiq152022016/kelembaban";  
const char* topicRelay = "Shidiq152022016/relay";     
const char* espConnectStat = "Shidiq152022016/esp";     

// Konfigurasi PIN
#define DHTPIN 4        
#define DHTTYPE DHT22    
#define greenLED 27     
#define yellowLED 14    
#define redLED 12     
#define relayPin 2      
#define buzzerPin 17    

// Inisialisasi
DHT dht(DHTPIN, DHTTYPE);
WiFiClient espClient;
PubSubClient client(espClient);

// Fungsi untuk menghubungkan ke WiFi
void wifiConnect() {
  delay(100);
  Serial.println();
  Serial.print("Menghubungkan ke WiFi");

  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi Terhubung");
}

// Fungsi untuk menghubungkan ke MQTT
void mqttConnect() {
  while (!client.connected()) {
    Serial.print("Menghubungkan ke MQTT...");
    if (client.connect(clientName)) {
      Serial.println("terhubung");
      client.publish(espConnectStat, "ESP Shidiq Connect");
    } else {
      Serial.print("Gagal, rc=");
      Serial.print(client.state());
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  // Inisialisasi pin
  pinMode(greenLED, OUTPUT);
  pinMode(yellowLED, OUTPUT);
  pinMode(redLED, OUTPUT);
  pinMode(relayPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);

  // Mulai koneksi ke WiFi
  wifiConnect();

  // Set MQTT server
  client.setServer(mqtt_server, 1883);

  // Mulai DHT sensor
  dht.begin();
}

void loop() {
  if (!client.connected()) {
    mqttConnect();
  }
  client.loop();

  // Baca suhu dan kelembaban
  float suhu = dht.readTemperature();
  float kelembaban = dht.readHumidity();
  if (isnan(suhu) || isnan(kelembaban)) {
    Serial.println("Gagal membaca sensor");
    return;
  }
  Serial.print("Suhu: ");
  Serial.println(suhu);
  Serial.print("kelembaban: ");
  Serial.print(kelembaban);

  // Pengkondisian suhu untuk LED, buzzer, dan relay
  if (suhu > 35) {
    digitalWrite(redLED, HIGH);
    digitalWrite(buzzerPin, HIGH);
    digitalWrite(relayPin, HIGH);  // Nyalakan pompa
    digitalWrite(greenLED, LOW);
    digitalWrite(yellowLED, LOW);
  } else if (suhu >= 30 && suhu <= 35) {
    digitalWrite(yellowLED, HIGH);
    digitalWrite(buzzerPin, LOW);
    digitalWrite(relayPin, LOW);  // Matikan pompa
    digitalWrite(greenLED, LOW);
    digitalWrite(redLED, LOW);
  } else {
    digitalWrite(greenLED, HIGH);
    digitalWrite(buzzerPin, LOW);
    digitalWrite(relayPin, LOW);  // Matikan pompa
    digitalWrite(yellowLED, LOW);
    digitalWrite(redLED, LOW);
  }

  // Kirim data suhu ke MQTT
  String suhuPayload = String(suhu);
  Serial.print("Mengirim suhu ke MQTT: ");
  Serial.println(suhuPayload);
  client.publish(topicSuhu, suhuPayload.c_str());

  // Kirim data kelembaban ke MQTT
  String kelembabanPayload = String(kelembaban);
  Serial.print("Mengirim kelembaban ke MQTT: ");
  Serial.println(kelembabanPayload);
  client.publish(topicKelembaban, kelembabanPayload.c_str());

  // Kirim status relay (pompa) ke MQTT
  String relayStatus = (suhu > 35) ? "ON" : "OFF";  // Pompa menyala jika suhu > 35°C
  Serial.print("Mengirim status relay ke MQTT: ");
  Serial.println(relayStatus);
  client.publish(topicRelay, relayStatus.c_str());

  // Delay 2 detik sebelum membaca lagi
  delay(5000);
}
