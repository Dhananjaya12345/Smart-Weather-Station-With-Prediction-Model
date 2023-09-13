#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <BH1750FVI.h>
#include <DHT.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ThingSpeak.h>

#define DHTPIN 12
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

#define BH1750_ADDR 0x23
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SSD1306_RESET 16
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

BH1750FVI LightSensor(BH1750FVI::k_DevModeContLowRes);
int rainSensor = A0;
const byte PulsesPerRevolution = 2;
const unsigned long ZeroTimeout = 100000;  // Timeout duration in microseconds
const byte numReadings = 2;

volatile unsigned long LastTimeWeMeasured;
volatile unsigned long PeriodBetweenPulses = ZeroTimeout + 1000;
volatile unsigned long PeriodAverage = ZeroTimeout + 1000;
unsigned long FrequencyRaw;

unsigned long FrequencyReal;
unsigned long RPM;
unsigned int PulseCounter = 1;
unsigned long PeriodSum;

unsigned long LastTimeCycleMeasure = LastTimeWeMeasured;
unsigned long CurrentMicros = micros();
unsigned int AmountOfReadings = 1;
unsigned int ZeroDebouncingExtra;
unsigned long readings[numReadings];
unsigned long readIndex;
unsigned long total;
unsigned long average;

const char* ssid = "HUAWEI Y6II";
const char* password = "dhananjaya";
const char* THINGSPEAK_CHANNEL_ID = "2165705";
const char* server = "api.thingspeak.com";
const char* apiKey = "8X9KHI2DQEZHYC5Q";
WiFiClient client;

void IRAM_ATTR Pulse_Event() {
  PeriodBetweenPulses = micros() - LastTimeWeMeasured;
  LastTimeWeMeasured = micros();

  // Add a delay to ignore any noise or false pulses
  delayMicroseconds(100);

  if (PulseCounter >= AmountOfReadings) {
    PeriodAverage = PeriodSum / AmountOfReadings;
    PulseCounter = 1;
    PeriodSum = PeriodBetweenPulses;

    int RemapedAmountOfReadings = map(PeriodBetweenPulses, 40000, 5000, 1, 10);
    RemapedAmountOfReadings = constrain(RemapedAmountOfReadings, 1, 10);
    AmountOfReadings = RemapedAmountOfReadings;
  } else {
    PulseCounter++;
    PeriodSum = PeriodSum + PeriodBetweenPulses;
  }

  // Print debug information to check if pulses are being detected
  Serial.println("Pulse detected");
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  ThingSpeak.begin(client);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  delay(2000);
  display.clearDisplay();
  display.setTextColor(WHITE);
  dht.begin();
  LightSensor.begin();

  
  attachInterrupt(digitalPinToInterrupt(14), Pulse_Event, RISING);  // Change to the desired GPIO pin for wind sensor

  // Pin configurations
  pinMode(A0, INPUT);
}

void loop() {
  // Read sensor data
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
  }
  float lux = LightSensor.GetLightIntensity();
  int rainValue = analogRead(A0); // Change to the desired pin for rain sensor
  bool isRaining = (rainValue < 500);

  LastTimeCycleMeasure = LastTimeWeMeasured;
  CurrentMicros = micros();
  if (CurrentMicros < LastTimeCycleMeasure) {
    LastTimeCycleMeasure = CurrentMicros;
  }

  // Check if timeout occurred (no pulses received within timeout duration)
  if (CurrentMicros - LastTimeCycleMeasure > ZeroTimeout) {
    FrequencyRaw = 0;  // Set frequency as 0
  } else {
    FrequencyRaw = 10000000000 / PeriodAverage;
  }

  FrequencyReal = FrequencyRaw / 10000;

  RPM = FrequencyRaw / PulsesPerRevolution * 60;
  RPM = RPM / 10000;

  total = total - readings[readIndex];
  readings[readIndex] = RPM;
  total = total + readings[readIndex];
  readIndex = readIndex + 1;

  if (readIndex >= numReadings) {
    readIndex = 0;
  }
  average = total / numReadings;
  // Circumference of the rotating object in kilometers
  const float Circumference = 0.5;  // Example: 0.5 kilometers
  // Calculate speed in km/h
  float speed = (RPM * Circumference) * (60.0 / 1000.0);

  // Build display buffer
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  // Temperature and humidity
  display.setCursor(0, 0);
  display.print("Temp: ");
  display.print(temperature, 1);
  display.println(" C");

  display.setCursor(0, 10);
  display.print("Humidity: ");
  display.print(humidity, 1);
  display.println(" %");

  // Light and rain
  display.setCursor(0, 20);
  display.print("Light: ");
  display.print(lux, 0);
  display.println(" LUX");

  display.setCursor(0, 30);
  display.print(isRaining ? "Raining" : "Clear Sky");

  // Wind speed
  display.setCursor(0, 40);
  display.print("Wind (km/h): ");
  display.print(speed, 1);

  // Display buffer
  display.display();

  // Send data to ThingSpeak 

  
  ThingSpeak.setField(1, temperature);
  ThingSpeak.setField(2, humidity);
   
  ThingSpeak.setField(3, lux);
  ThingSpeak.setField(4, isRaining);
  ThingSpeak.setField(5, speed);
  int httpCode = ThingSpeak.writeFields(2165705, apiKey);
  if (httpCode == 200) {
    Serial.println("Data sent to ThingSpeak");
  } else {
    Serial.println("Failed to send data to ThingSpeak");
  }

  delay(10000);
}
