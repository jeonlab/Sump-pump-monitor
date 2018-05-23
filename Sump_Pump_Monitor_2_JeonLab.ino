#include <ESP8266WiFi.h>
#include "Gsender.h"

#pragma region Globals
const char* ssid = "your SSID";
const char* password = "your wifi password";
const char* email = "your email address";
uint8_t connection_state = 0;        // Connected to WIFI or not
uint16_t reconnect_interval = 10000; // If not connected wait time to try again
#pragma endregion Globals

int trigPin = 2;
int echoPin = 0;
int level_high = 100;
int level_low = 0;
int previous_cm, count = 0;
int cm;
int pumpIntervalMin;
unsigned long lastPumpOnMillis;
const long milsec_to_min = 60000;

String subject, message, data;

uint8_t WiFiConnect(const char* nSSID = nullptr, const char* nPassword = nullptr)
{
  static uint16_t attempt = 0;
  if (nSSID) {
    WiFi.begin(nSSID, nPassword);
  } else {
    WiFi.begin(ssid, password);
  }

  uint8_t i = 0;
  while (WiFi.status() != WL_CONNECTED && i++ < 50)
  {
    delay(200);
  }
  ++attempt;
  if (i == 51) {
    if (attempt % 2 == 0) return false;
  }
  return true;
}

void Awaits()
{
  uint32_t ts = millis();
  while (!connection_state)
  {
    delay(50);
    if (millis() > (ts + reconnect_interval) && !connection_state) {
      connection_state = WiFiConnect();
      ts = millis();
    }
  }
}

void setup()
{
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  connection_state = WiFiConnect();
  if (!connection_state) // if not connected to WIFI
    Awaits();          // constantly trying to connect

  Gsender *gsender = Gsender::Instance();    // Getting pointer to class instance
  subject = "Monitor started";
  gsender->Subject(subject)->Send(email, "");
}

void loop() {
  cm = measure_cm();
  if (previous_cm == 0) previous_cm = cm;
  else if (cm - previous_cm > 10) {
    pumpIntervalMin = int((millis()-lastPumpOnMillis)/milsec_to_min);
    lastPumpOnMillis = millis();
  }

  data += " " + String(cm);
  Gsender *gsender = Gsender::Instance();

  if (cm < 25) {
    subject = "WARNING!!!  Sump water level HIGH: " + String(cm);
    message = "High: " + String(level_high) + "  Low: " + String(level_low);
    gsender->Subject(subject)->Send(email, message);
  }

  if (count > 50) {
    subject = String(cm) + " (" + String(level_high) + "/" + String(level_low) + ") " 
              + String(pumpIntervalMin) + " min";
    message = data;
    gsender->Subject(subject)->Send(email, message);
    count = 0;
    data = "";
    level_high = 100;
    level_low = 0;
  }
  if (cm < level_high) level_high = cm;
  if (cm > level_low) level_low = cm;
  previous_cm = cm;
  count++;
  delay(milsec_to_min);
}

int measure_cm() {
  long duration;
  int cm, cm_sum = 0, cm_min = 0, cm_max = 0, count = 0;

  do {
    // trigger 10us pulse
    digitalWrite(trigPin, 0);
    delayMicroseconds(5);
    digitalWrite(trigPin, 1);
    delayMicroseconds(10);
    digitalWrite(trigPin, 0);

    duration = pulseIn(echoPin, 0);

    // convert the time into a distance
    cm = duration / 58;
    if (count == 0) {
      cm_min = cm;
      cm_max = cm;
    }
    else {
      if (cm < cm_min) cm_min = cm;
      if (cm > cm_max) cm_max = cm;
    }
    cm_sum = cm_sum + cm;

    count++;
    delay(1000);
  } while (count < 10);

  cm_sum = cm_sum - cm_min - cm_max;
  cm = cm_sum / 8;

  return cm;
}

