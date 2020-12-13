// Loading the ESP8266WiFi library and the PubSubClient library
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include "DHTesp.h"

DHTesp dht;

// Change the credentials below, so your ESP8266 connects to your router
const char* ssid = "wifi-name";
const char* password = "password";

// Change the variable to your Raspberry Pi IP address, so it connects to your MQTT broker
const char* mqtt_server = "ip-address";

// Initializes the espClient
WiFiClient espClient;
PubSubClient client(espClient);

/*
D0  16
D1  5
D2  4
D3  0
D4  2
D5  14
D6  12
D7  13
D8  15
TX  1
RX  3
*/

// DHT Sensor
const int DHTPin = 12; // D6

#define BME_SCK D1;// SCK to D1 on Wemos D-1 Mini
#define BME_CS D2;// SDI to D2 on Wemos D-1 Mini

#define SEALEVELPRESSURE_HPA (1013.25)
Adafruit_BME280 bme; // I2C

// Time to sleep (in seconds):
const int sleepTimeS = 60;

// JSON objects
StaticJsonBuffer<256> JSONbuffer;
JsonObject& JSONencoder = JSONbuffer.createObject();

// calibration of BME280 with respect to Weather Station (Vlinder 19 Brussels https://wow.meteo.be
//const float pressureFit_a = 0.9728;
//const float pressureFit_b = 28.8216;
// calibration of BME280 with respect to DHT11 sensor. DHT11 sensor and Weather Station have very similar measurements
//const float humidityFit_a = 1.6916;
//const float humidityFit_b = -10.5694;
//const float temperatureFit_a = 0.9914;
//const float temperatureFit_b = -1.3789;

// Connects your ESP8266 to your router
void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi connected - ESP IP address: ");
  Serial.println(WiFi.localIP());
}

// This functions is executed when some device publishes a message to a topic that your ESP8266 is subscribed to
// Change the function below to add logic to your program, so when a device publishes a message to a topic that
// your ESP8266 is subscribed you can actually do something
void callback(String topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;

  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }

  Serial.println();
}

// This functions reconnects your ESP8266 to your MQTT broker
// Change the function below if you want to subscribe to more topics with your ESP8266
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
     /*
     YOU  NEED TO CHANGE THIS NEXT LINE, IF YOU'RE HAVING PROBLEMS WITH MQTT MULTIPLE CONNECTIONS
     To change the ESP device ID, you will have to give a unique name to the ESP8266.
     Here's how it looks like now:
       if (client.connect("ESP8266Client")) {
     If you want more devices connected to the MQTT broker, you can do it like this:
       if (client.connect("ESPOffice")) {
     Then, for the other ESP:
       if (client.connect("ESPGarage")) {
      That should solve your MQTT multiple connections problem

     THE SECTION IN loop() function should match your device name
    */
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");

      // You can subscribe to more topics (to control more LEDs in this example)
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

// The setup function sets your ESP GPIOs to Outputs, starts the serial communication at a baud rate of 115200
// Sets your mqtt broker and sets the callback function
// The callback function is what receives messages and actually controls the LEDs
void setup() {
  dht.setup(DHTPin, DHTesp::DHT11);

  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

 // pinMode(4, INPUT_PULLUP); //Set input (SDA) pull-up resistor on
  //Wire.begin(4,5); // Define which ESP8266 pins to use for SDA, SCL of the Sensor
//
  unsigned status;
  // default settings
  //status = bme.begin();
    // You can also pass in a Wire library object like &Wire2
  status = bme.begin(0x76);
  if (!status) {
    Serial.println("Could not find a valid BME280 sensor, check wiring, address, sensor ID!");
    Serial.print("SensorID was: 0x"); Serial.println(bme.sensorID(),16);
    Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
    Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
    Serial.print("        ID of 0x60 represents a BME 280.\n");
    Serial.print("        ID of 0x61 represents a BME 680.\n");
    while (1) delay(10);
  }

  runSensors();

  ESP.deepSleep(sleepTimeS * 1000000,WAKE_RF_DEFAULT);   //try the default mode
  delay(100);
}

void runSensors()
{
  if (!client.connected()) {
    reconnect();
  }
  if(!client.loop())
     /*
     YOU  NEED TO CHANGE THIS NEXT LINE, IF YOU'RE HAVING PROBLEMS WITH MQTT MULTIPLE CONNECTIONS
     To change the ESP device ID, you will have to give a unique name to the ESP8266.
     Here's how it looks like now:
       client.connect("ESP8266Client");
     If you want more devices connected to the MQTT broker, you can do it like this:
       client.connect("ESPOffice");
     Then, for the other ESP:
       client.connect("ESPGarage");
      That should solve your MQTT multiple connections problem

     THE SECTION IN recionnect() function should match your device name
    */
  client.connect("ESP8266Client");

  JSONencoder["location"] = "OUTSIDE-WINDOW";

  runDHTsensor();
  runBMEsensor();
}

void runDHTsensor()
{
  // DHT11 measurement
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float hDht = dht.getHumidity();
  // Read temperature as Celsius (the default)
  float tDht = dht.getTemperature();

  Serial.print(dht.getStatusString());
  // Check if any reads failed and exit early (to try again).
  if (isnan(hDht) || isnan(tDht)) {
    Serial.print(hDht);
    Serial.print(tDht);
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  float hicDht = dht.computeHeatIndex(tDht, hDht, false);
  float dewPointDht = tDht - ((100 - hDht)/5.0F);

  JSONencoder["device"] = "DHT11";

  // TEMPERATURES DHT11
  JSONencoder["unit"] = "°C";

  // TEMPERATURE
  JSONencoder["type"] = "Temperature";
  JSONencoder["value"] = tDht;

  char JSONmessageBufferTemperature[256];
  JSONencoder.printTo(JSONmessageBufferTemperature, sizeof(JSONmessageBufferTemperature));
  Serial.println(JSONmessageBufferTemperature);
  if(client.publish("/weather/temperature/dht11", JSONmessageBufferTemperature) == true)
  {
    Serial.println("Success sending temperature message");
  } else {
    Serial.println("Error sending temperature message");
  }

  // DEW POINT
  JSONencoder["type"] = "Dew Point";
  JSONencoder["value"] = dewPointDht;

  char JSONmessageBufferDewPoint[256];
  JSONencoder.printTo(JSONmessageBufferDewPoint, sizeof(JSONmessageBufferDewPoint));
  Serial.println(JSONmessageBufferDewPoint);
  if(client.publish("/weather/dew_point/dht11", JSONmessageBufferDewPoint) == true)
  {
    Serial.println("Success sending dew point message");
  } else {
    Serial.println("Error sending dew point message");
  }

  // HEAT INDEX
  JSONencoder["type"] = "Heat Index";
  JSONencoder["value"] = hicDht;

  char JSONmessageBufferHeatIndex[256];
  JSONencoder.printTo(JSONmessageBufferHeatIndex, sizeof(JSONmessageBufferHeatIndex));
  Serial.println(JSONmessageBufferHeatIndex);
  if(client.publish("/weather/heat_index/dht11", JSONmessageBufferHeatIndex) == true)
  {
    Serial.println("Success sending heat index message");
  } else {
    Serial.println("Error sending heat index message");
  }

  // HUMIDITY
  JSONencoder["unit"] = "%";
  JSONencoder["type"] = "Humidity";
  JSONencoder["value"] = hDht;

  char JSONmessageBufferHumidity[256];
  JSONencoder.printTo(JSONmessageBufferHumidity, sizeof(JSONmessageBufferHumidity));
  Serial.println(JSONmessageBufferHumidity);
  if(client.publish("/weather/humidity/dht11", JSONmessageBufferHumidity) == true)
  {
    Serial.println("Success sending humidity message");
  } else {
    Serial.println("Error sending humidity message");
  }

  // Log measurements
  printDHTValues(hDht, tDht, hicDht, dewPointDht);
}

void runBMEsensor()
{
  // BME280 measurement is calibrated with respect to weather station and DHT11 sensor
  //float tBme = bme.readTemperature() * temperatureFit_a + temperatureFit_b;
  //float pBme = (bme.readPressure() / 100.0F)*pressureFit_a + pressureFit_b;
  //float hBme = bme.readHumidity() * humidityFit_a + humidityFit_b;
  float tBme = bme.readTemperature();
  float pBme = bme.readPressure() / 100.0F;
  float hBme = bme.readHumidity();
  float altitudeBme = bme.readAltitude(SEALEVELPRESSURE_HPA);
  float dewPointBme = tBme - ((100 - hBme)/5.0F);

  JSONencoder["device"] = "BME280";

  // TEMPERATURES
  JSONencoder["unit"] = "°C";

  // TEMPERATURE
  JSONencoder["type"] = "Temperature";
  JSONencoder["value"] = tBme;

  char JSONmessageBufferTemperature[256];
  JSONencoder.printTo(JSONmessageBufferTemperature, sizeof(JSONmessageBufferTemperature));
  Serial.println(JSONmessageBufferTemperature);
  if(client.publish("/weather/temperature/bme280", JSONmessageBufferTemperature) == true)
  {
    Serial.println("Success sending temperature message");
  } else {
    Serial.println("Error sending temperature message");
  }

  // DEW POINT
  JSONencoder["type"] = "Dew Point";
  JSONencoder["value"] = dewPointBme;

  char JSONmessageBufferDewPoint[256];
  JSONencoder.printTo(JSONmessageBufferDewPoint, sizeof(JSONmessageBufferDewPoint));
  Serial.println(JSONmessageBufferDewPoint);
  if(client.publish("/weather/dew_point/bme280", JSONmessageBufferDewPoint) == true)
  {
    Serial.println("Success sending dew point message");
  } else {
    Serial.println("Error sending dew point message");
  }

  // HUMIDITY
  JSONencoder["unit"] = "%";
  JSONencoder["type"] = "Humidity";
  JSONencoder["value"] = hBme;

  char JSONmessageBufferHumidity[256];
  JSONencoder.printTo(JSONmessageBufferHumidity, sizeof(JSONmessageBufferHumidity));
  Serial.println(JSONmessageBufferHumidity);
  if(client.publish("/weather/humidity/bme280", JSONmessageBufferHumidity) == true)
  {
    Serial.println("Success sending humidity message");
  } else {
    Serial.println("Error sending humidity message");
  }

  // PRESSURE
  JSONencoder["unit"] = "hPa";
  JSONencoder["type"] = "Pressure";
  JSONencoder["value"] = pBme;

  char JSONmessageBufferPressure[256];
  JSONencoder.printTo(JSONmessageBufferPressure, sizeof(JSONmessageBufferPressure));
  Serial.println(JSONmessageBufferPressure);
  if(client.publish("/weather/pressure/bme280", JSONmessageBufferPressure) == true)
  {
    Serial.println("Success sending humidity message");
  } else {
    Serial.println("Error sending humidity message");
  }

  // Log measurements
  printBMTValues(tBme, pBme, altitudeBme, hBme, dewPointBme);
}

// For this project, you don't need to change anything in the loop function.
// Basically it ensures that you ESP is connected to your broker
void loop() {
}

void printBMTValues(float t, float p, float alt, float h, float dewPoint) {
  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.print(" %\t Temperature: ");
  Serial.print(t);
  Serial.print(" *C ");
  Serial.print("\t Dew Point: ");
  Serial.print(dewPoint);
  Serial.print(" *C ");
  Serial.print("\t Pressure: ");
  Serial.print(p);
  Serial.print(" hpa ");
  Serial.print("\t Approx. Altitude: ");
  Serial.print(alt);
  Serial.println(" m ");
}

void printDHTValues(float h, float t, float hic, float dewPoint) {
  Serial.println();
  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.print(" %\t Temperature: ");
  Serial.print(t);
  Serial.print(" *C ");
  Serial.print("\t Dew Point: ");
  Serial.print(dewPoint);
  Serial.print(" *C ");
  Serial.print("\t Heat index: ");
  Serial.print(hic);
  Serial.println(" *C ");
}
