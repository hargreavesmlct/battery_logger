#include <OneWire.h>
#include <DallasTemperature.h>

#include <Wire.h>
#include <Adafruit_INA219.h>

#include <Adafruit_SSD1306.h>

#include <virtuabotixRTC.h>

#include <SD.h>
#include <SPI.h>

#include <WiFi.h>
#include "ThingSpeak.h"

#define SD_CS_PIN 5
File dataFile;

Adafruit_INA219 ina219;

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

OneWire wiring(14);
DallasTemperature sensor(&wiring);

#define PIN_CLK 15
#define PIN_DAT 2
#define PIN_RST 4
virtuabotixRTC myRTC(PIN_CLK, PIN_DAT, PIN_RST);


unsigned long previousOledMillis = 0;
unsigned long oledInterval = 5000;  // Perbarui interval menjadi 5000 (5 detik)

unsigned long previousSerialMillis = 0;
unsigned long serialInterval = 1000;  // Perbarui interval Serial Monitor sesuai kebutuhan

unsigned long previousSaveMillis = 0;
unsigned long saveInterval = 1000; //Perbaharui interval

unsigned long previousThingSpeakMillis = 0;
unsigned long ThingSpeakInterval = 15000; //Perbaharui interval

int displayState = 0;  // Menyimpan status tampilan OLED saat ini

float full_busvoltage = 0; // Deklarasi variabel global full_busvoltage
float current_mA = 0; // Deklarasi variabel global current_mA
float power_mW = 0; // Deklarasi variabel global power_mW
float total_capacity = 0; // Deklarasi variabel global total_capacity

float dataSuhu = 0; // Deklarasi variabel global dataSuhu

const int relayPin = 32; // Deklarasi Relay

String jam_sekarang; // Deklarasi variabel global jam_sekarang
String tgl_sekarang; // Deklarasi variabel global tgl_sekarang
String hari; // Deklarasi variabel global hari

const char* ssid = "LAB_ELDA"; // Ganti dengan SSID Wifi anda
const char* password = "elda123456"; // Ganti dengan PASSWORD Wifi anda

unsigned long myChannelNumber = 2206734; // Ganti dengan nomor channel Thingspeak Anda
const char* myWriteAPIKey = "QOQX3LGFETUU89JU"; // Ganti dengan API Key Thingspeak Anda

WiFiClient client;

void setup() {

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();

  pinMode(relayPin, OUTPUT);

  Serial.begin(115200);
  
  if (!ina219.begin()) {
    Serial.println("Failed to find INA219 chip");
    while (1) {
      delay(10);
    }
  }

  if (! ina219.begin()) {
    Serial.println("Failed to find INA219 chip");
    while (1) { delay(10); }
  }

  Serial.println("Measuring voltage and current with INA219 ...");

  sensor.begin();

  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("Gagal memuat SD card");
    return;
  }
  Serial.println("SD card terdeteksi");

  dataFile = SD.open("/Baterai.csv", FILE_WRITE);
  if (!dataFile) {
    Serial.println("Gagal membuka file");
    return;
  }
  dataFile.println("Hari, Tanggal, Waktu, Tegangan(V), Arus(mA), Daya(mW), Kapasitas Digunakan(mAh), Suhu");
  dataFile.close();

  // DETIK, MENIT, JAM, HARI, TANGGAL, BULAN, TAHUN
  // myRTC.setDS1302Time (00, 27, 11, 5, 16, 6, 2023);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi!");

  // Inisialisasi koneksi Thingspeak
  ThingSpeak.begin(client);
}

void loop() {
  safetyProtection();
  updateOledDisplay();
  rtcUpdate();
  updateSensor();
  dataLogger();
  dataCloud();
}

void updateOledDisplay() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousOledMillis >= oledInterval) {
    previousOledMillis = currentMillis;
    
    if (dataSuhu >=55){
      display.clearDisplay();
      display.display();
    } else {
    switch (displayState) {
      case 0:
        {
          display.clearDisplay();
          display.setTextSize(1.9);
          display.setTextColor(WHITE);
          display.setCursor(2, 30);
          display.print("Tegangan: ");

          display.setTextSize(2);
          display.setTextColor(WHITE);
          display.setCursor(2, 45);
          display.print(full_busvoltage);
          display.println(" V");

          display.display();
          break;
        }

      case 1:
        {
          display.clearDisplay();
          display.setTextSize(1.9);
          display.setTextColor(WHITE);
          display.setCursor(2, 30);
          display.print("Arus: ");

          display.setTextSize(2);
          display.setTextColor(WHITE);
          display.setCursor(2, 45);
          display.print(current_mA);
          display.println(" mA");

          display.display();
          break;
        }

      case 2:
        {
          display.clearDisplay();
          display.setTextSize(1.9);
          display.setTextColor(WHITE);
          display.setCursor(2, 30);
          display.print("Daya: ");

          display.setTextSize(2);
          display.setTextColor(WHITE);
          display.setCursor(2, 45);
          display.print(power_mW);
          display.println(" mW");

          display.display();
          break;
        }

      case 3:
        {
          display.clearDisplay();
          display.setTextSize(1.9);
          display.setTextColor(WHITE);
          display.setCursor(2, 30);
          display.print("Kapasitas Digunakan: ");

          display.setTextSize(2);
          display.setTextColor(WHITE);
          display.setCursor(2, 45);
          display.print(total_capacity);
          display.println("mAh");

          display.display();
          break;
        }

      case 4:
        {
          display.clearDisplay();
          display.setTextSize(1.9);
          display.setTextColor(WHITE);
          display.setCursor(2, 30);
          display.print("Suhu: ");

          display.setTextSize(2);
          display.setTextColor(WHITE);
          display.setCursor(2, 45);
          display.print(dataSuhu);
          display.println(" C");

          display.display();
          break;
        }
    }
  }
    displayState++;
    if (displayState > 4) {
      displayState = 0;
    }
  }
}

void rtcUpdate() {
  myRTC.updateTime();

  String jam, menit, detik, tanggal, bulan, tahun;

  hari = myRTC.dayofweek;
  if (hari == "1") hari = "Senin";
  else if (hari == "2") hari = "Selasa";
  else if (hari == "3") hari = "Rabu";
  else if (hari == "4") hari = "Kamis";
  else if (hari == "5") hari = "Jumat";
  else if (hari == "6") hari = "Sabtu";
  else if (hari == "7") hari = "Minggu";

  jam = myRTC.hours;
  menit = myRTC.minutes;
  detik = myRTC.seconds;
  jam_sekarang = jam + ":" + menit + ":" + detik;

  tanggal = myRTC.dayofmonth; // Menggunakan dayofmonth sebagai tanggal
  bulan = myRTC.month;
  tahun = myRTC.year;
  tgl_sekarang = tanggal + "/" + bulan + "/" + tahun;

  display.setTextSize(1.8);
  display.setTextColor(WHITE);
  display.setCursor(0, 1);
  display.print(hari + ", " + tanggal + " / " + bulan + " / " + tahun);

  display.setTextSize(2.1);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
  display.print(jam + ":" + menit);

  display.display();
}

void updateSensor() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousSerialMillis >= serialInterval) {
    previousSerialMillis = currentMillis;

    sensor.setResolution(11);
    sensor.requestTemperatures();
    dataSuhu = sensor.getTempCByIndex(0);

    float shuntvoltage = ina219.getShuntVoltage_mV();
    float busvoltage = ina219.getBusVoltage_V();
    current_mA = ina219.getCurrent_mA(); // Update variabel current_mA
    power_mW = ina219.getPower_mW(); // Update variabel power_mW

    float loss_percentage = 1;
    float lost_data = (loss_percentage / 100.0) * busvoltage;
    full_busvoltage = busvoltage + lost_data; // Update variabel full_busvoltage
    float capacity = (current_mA * serialInterval) / (1000 * 3600);
    total_capacity += capacity;

    Serial.print("Bus Voltage      : ");
    Serial.print(full_busvoltage);
    Serial.println(" V");

    Serial.print("Shunt Voltage    : ");
    Serial.print(shuntvoltage);
    Serial.println(" mV");

    Serial.print("Current          : ");
    Serial.print(current_mA);
    Serial.println(" mA");
    
    Serial.print("Power            : ");
    Serial.print(power_mW);
    Serial.println(" mW");

    Serial.print("Capacity Used    : ");
    Serial.print(total_capacity);
    Serial.println(" mAh");

    Serial.print("Suhu             : ");
    Serial.print(dataSuhu);
    Serial.println(" Celcius");
    Serial.println("");
  }
}

void dataLogger(){

  unsigned long saveMillis = millis();

  if (saveMillis - previousSaveMillis >= saveInterval) {
    previousSaveMillis = saveMillis;

  dataFile = SD.open("/Baterai.csv", FILE_APPEND);
  if (dataFile) { // cek apakah file sudah ada
    dataFile.print(hari);
    dataFile.print(",");
    dataFile.print(tgl_sekarang);
    dataFile.print(",");
    dataFile.print(jam_sekarang);
    dataFile.print(",");
    dataFile.print(full_busvoltage);
    dataFile.print(",");
    dataFile.print(current_mA);
    dataFile.print(",");
    dataFile.print(power_mW);
    dataFile.print(",");
    dataFile.print(total_capacity);
    dataFile.print(",");
    dataFile.println(dataSuhu);
    dataFile.close();
    Serial.println("Success Save");
  } 
  else { // jika belum ada, maka buat file baru
  dataFile = SD.open("/Baterai.csv", FILE_WRITE);
  if (dataFile) {
    dataFile.print(hari);
    dataFile.print(",");
    dataFile.print(tgl_sekarang);
    dataFile.print(",");
    dataFile.print(jam_sekarang);
    dataFile.print(",");
    dataFile.print(full_busvoltage);
    dataFile.print(",");
    dataFile.print(current_mA);
    dataFile.print(",");
    dataFile.print(power_mW);
    dataFile.print(",");
    dataFile.print(total_capacity);
    dataFile.print(",");
    dataFile.println(dataSuhu);
    dataFile.close();
    Serial.println("Success Create & Save");
  } 
  else {
    Serial.println("Failed Create & Save!!!");
      }
    }
  }
}

void dataCloud(){
  unsigned long ThingSpeakMillis = millis();

  if (ThingSpeakMillis - previousThingSpeakMillis >= ThingSpeakInterval) {
    previousThingSpeakMillis = ThingSpeakMillis;

    if(full_busvoltage >= 5){
      ThingSpeak.setField(1, full_busvoltage);
      ThingSpeak.setField(2, current_mA);
      ThingSpeak.setField(3, power_mW);
      ThingSpeak.setField(4, total_capacity);
      ThingSpeak.setField(5, dataSuhu);

      int httpCode = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
      if (httpCode == 200) {
        Serial.println("Data berhasil dikirim ke Thingspeak!");
      } else {
        Serial.println("Gagal mengirim data ke Thingspeak. Kode HTTP: " + String(httpCode));
      }
    } else {
      Serial.println("Bus voltage is under 5 V");
    }
  }
}

void safetyProtection(){

  if(dataSuhu >=55){
    digitalWrite(relayPin, LOW);

    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(2, 40);
    display.print("Overheat");
    display.display();
    
  } else {
    digitalWrite(relayPin,HIGH);
  }
}
