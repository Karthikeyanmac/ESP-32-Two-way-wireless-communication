#include <LiquidCrystal_I2C.h>
#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>
#include <DHT.h>

#define DHTPin 15
#define DHTType DHT11

DHT dht(DHTPin, DHTType);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Receiver MAC Address
uint8_t broadcastAddress[] = {0x6C, 0xC8, 0x40, 0x34, 0xB0, 0x50};

// Global sensor values
float h;
float t;
float f;

// Incoming values (if needed)
float incomingH;
float incomingT;
float incomingF;

String success;

// Structure for ESP-NOW
typedef struct struct_message {
  float h;
  float t;
  float f;
} struct_message;

struct_message dhtReadings;
struct_message incomingReadings;

esp_now_peer_info_t peerInfo;

// ================= CALLBACKS =================

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
  if (status ==0){
    success = "Delivery Success :)";
  }
  else{
    success = "Delivery Fail :(";
  }
}

// Receive Callback
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));
  incomingT = incomingReadings.t;
  incomingH = incomingReadings.h;
  incomingF = incomingReadings.f;
}

// ================= SETUP =================

void setup() {
  Serial.begin(9600);

  lcd.init();
  lcd.clear();
  lcd.backlight();

  dht.begin();

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Init Failed");
    return;
  }

  esp_now_register_send_cb(esp_now_send_cb_t(OnDataSent));
  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));

  // Clear peer info before use
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
}

// ================= LOOP =================

void loop() {
  getReadings();

  dhtReadings.t = t;
  dhtReadings.h = h;
  dhtReadings.f = f;

  esp_err_t result = esp_now_send(broadcastAddress,(uint8_t *) &dhtReadings,sizeof(dhtReadings));

  if (result == ESP_OK) {
    Serial.println("Data Sent Successfully");
  } else {
    Serial.println("Error Sending Data");
  }

  delay(10000);
}

// ================= SENSOR FUNCTION =================

void getReadings() {
  h = dht.readHumidity();
  t = dht.readTemperature();
  f = dht.readTemperature(true);

  float hif = dht.computeHeatIndex(f, h);       // Fahrenheit
  float hic = dht.computeHeatIndex(t, h, false); // Celsius

  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println("DHT Read Failed");

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("DHT Error");
    return;
  }

  // LCD DISPLAY
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp:");
  lcd.print(t);
  lcd.print("C");

  lcd.setCursor(0, 1);
  lcd.print("Hum:");
  lcd.print(h);
  lcd.print("%");

  // SERIAL MONITOR
  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.print("%  Temp: ");
  Serial.print(t);
  Serial.print("C  Heat Index C: ");
  Serial.print(hic);
  Serial.print("  Heat Index F: ");
  Serial.println(hif);
}
