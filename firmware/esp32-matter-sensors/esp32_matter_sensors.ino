#include <Arduino.h>
#include <Matter.h>
#include <Wire.h>
#include <WiFi.h>

#include <Adafruit_AM2320.h>
#include <DallasTemperature.h>
#include <OneWire.h>

namespace {

constexpr uint8_t kDs18b20Pin = 4;
constexpr uint8_t kI2cSdaPin = 19;
constexpr uint8_t kI2cSclPin = 21;
constexpr uint32_t kSensorPollIntervalMs = 10000;
constexpr uint32_t kDecommissionHoldMs = 5000;

struct WiFiCredential {
  const char *ssid;
  const char *password;
};

constexpr WiFiCredential kNetworks[] = {
    {"Naiit-TP-Link", "24031993"},
    {"Tenda", "24031993"},
};

OneWire oneWire(kDs18b20Pin);
DallasTemperature ds18b20(&oneWire);
Adafruit_AM2320 am2320(&Wire);

MatterTemperatureSensor ds18b20Matter;
MatterTemperatureSensor am2320TemperatureMatter;
MatterHumiditySensor am2320HumidityMatter;

bool am2320Available = false;
bool ds18b20Available = false;
DeviceAddress ds18b20Address = {0};

uint32_t lastSensorPollAt = 0;
uint32_t buttonPressedAt = 0;
bool buttonPressed = false;

bool isValidTemperature(float value) {
  return !isnan(value) && value > -55.0f && value < 125.0f;
}

bool isValidHumidity(float value) {
  return !isnan(value) && value >= 0.0f && value <= 100.0f;
}

void printCommissioningInfoIfNeeded() {
  if (Matter.isDeviceCommissioned()) {
    Serial.println("Matter node commissioned and ready.");
    return;
  }

  Serial.println();
  Serial.println("Matter node is not commissioned yet.");
  Serial.printf("Manual pairing code: %s\n", Matter.getManualPairingCode().c_str());
  Serial.printf("QR code URL: %s\n", Matter.getOnboardingQRCodeUrl().c_str());
}

bool connectToWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(false);

  for (const WiFiCredential &network : kNetworks) {
    Serial.printf("Connecting to Wi-Fi SSID '%s'...\n", network.ssid);
    WiFi.begin(network.ssid, network.password);

    const uint32_t startedAt = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startedAt < 20000) {
      delay(500);
      Serial.print('.');
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
      Serial.printf("Connected to %s, IP: %s\n", network.ssid,
                    WiFi.localIP().toString().c_str());
      return true;
    }

    Serial.printf("Failed to connect to %s\n", network.ssid);
    WiFi.disconnect(true, true);
    delay(1000);
  }

  Serial.println("Unable to connect to configured Wi-Fi networks.");
  return false;
}

void setupMatterEndpoints() {
  ds18b20Matter.begin(0.0);
  am2320TemperatureMatter.begin(0.0);
  am2320HumidityMatter.begin(0.0);
}

void setupSensors() {
  Wire.begin(kI2cSdaPin, kI2cSclPin);
  Wire.setClock(100000);

  am2320Available = am2320.begin();
  Serial.printf("AM2320 status: %s\n", am2320Available ? "detected" : "not detected");

  ds18b20.begin();
  ds18b20.setWaitForConversion(true);
  ds18b20Available = ds18b20.getAddress(ds18b20Address, 0);
  Serial.printf("DS18B20 status: %s\n", ds18b20Available ? "detected" : "not detected");
}

void publishSensorData() {
  if (ds18b20Available) {
    ds18b20.requestTemperatures();
    const float dsTemp = ds18b20.getTempC(ds18b20Address);
    if (isValidTemperature(dsTemp)) {
      ds18b20Matter.setTemperature(dsTemp);
      Serial.printf("DS18B20 temperature: %.2f C\n", dsTemp);
    } else {
      Serial.println("DS18B20 read failed.");
    }
  }

  if (am2320Available) {
    const float amTemp = am2320.readTemperature();
    const float amHumidity = am2320.readHumidity();

    if (isValidTemperature(amTemp)) {
      am2320TemperatureMatter.setTemperature(amTemp);
      Serial.printf("AM2320 temperature: %.2f C\n", amTemp);
    } else {
      Serial.println("AM2320 temperature read failed.");
    }

    if (isValidHumidity(amHumidity)) {
      am2320HumidityMatter.setHumidity(amHumidity);
      Serial.printf("AM2320 humidity: %.2f %%\n", amHumidity);
    } else {
      Serial.println("AM2320 humidity read failed.");
    }
  }
}

void handleButtonDecommission() {
  const bool isPressedNow = digitalRead(BOOT_PIN) == LOW;

  if (isPressedNow && !buttonPressed) {
    buttonPressed = true;
    buttonPressedAt = millis();
  }

  if (!isPressedNow && buttonPressed) {
    buttonPressed = false;
  }

  if (buttonPressed && millis() - buttonPressedAt >= kDecommissionHoldMs) {
    Serial.println("Decommissioning Matter node.");
    Matter.decommission();
    buttonPressedAt = millis();
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(BOOT_PIN, INPUT_PULLUP);

  connectToWiFi();
  setupSensors();
  setupMatterEndpoints();

  Matter.onEvent([](matterEvent_t event, const chip::DeviceLayer::ChipDeviceEvent *) {
    Serial.printf("Matter event: 0x%04X\n", event);
  });

  Matter.begin();
  printCommissioningInfoIfNeeded();
  publishSensorData();
  lastSensorPollAt = millis();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectToWiFi();
  }

  if (millis() - lastSensorPollAt >= kSensorPollIntervalMs) {
    publishSensorData();
    lastSensorPollAt = millis();
  }

  handleButtonDecommission();
  delay(50);
}
