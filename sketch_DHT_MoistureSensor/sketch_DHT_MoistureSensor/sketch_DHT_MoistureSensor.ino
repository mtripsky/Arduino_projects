#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "DHTesp.h"

const char* ssid = "wifi-naame";
const char* password = "password";

// Change the variable to your Raspberry Pi IP address, so it connects to your MQTT broker
const char* mqtt_server = "ip-address";

// Initializes the espClient
WiFiClient espClient;
PubSubClient client(espClient);

// Moisture sensor
const int AirValue = 667;
const int WaterValue = 264;
int soilMoistureValue = 0;
int soilmoisturepercent = 0;

// DHT Sensor
DHTesp dht;
const int DHTPin = 5;

// Time to sleep (in seconds):
const int sleepTimeS = 60;

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
    if (client.connect("ESP8266LivingRoomClient")) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  dht.setup(DHTPin, DHTesp::DHT22);
  Serial.begin(115200); // open serial port, set the baud rate to 9600 bps
  setup_wifi();
  client.setServer(mqtt_server, 1883);

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

  client.connect("ESP8266LivingRoomClient");

  runDHTsensor();
  runMoistureSensor();
}

void runDHTsensor()
{
  // DHT22 measurement
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

  // Publishes Temperature and Humidity values for DHT22
  static char temperatureTempDht[7];
  dtostrf(hicDht, 6, 2, temperatureTempDht);
  static char humidityTempDht[7];
  dtostrf(hDht, 6, 2, humidityTempDht);

  client.publish("/esp8266LivingRoom/dht22-temperature", temperatureTempDht);
  client.publish("/esp8266LivingRoom/dht22-humidity", humidityTempDht);

  // Log measurements
  printDHTValues(hDht, tDht, hicDht);
}

void runMoistureSensor() {
  soilMoistureValue = analogRead(A0);  //put Sensor insert into soil
  soilmoisturepercent = map(soilMoistureValue, AirValue, WaterValue, 0, 100);

  if(soilmoisturepercent > 100)
  {
    soilmoisturepercent = 100;
  }
  else if(soilmoisturepercent < 0)
  {
    soilmoisturepercent = 0;
  }

  static char soilMoisture[5];
  dtostrf(soilmoisturepercent, 4, 0, soilMoisture);

  client.publish("/esp8266LivingRoom/soil-moisture", soilMoisture);

  Serial.print("Soil Moisture: ");
  Serial.print(soilmoisturepercent);
  Serial.println(" %");
}

void printDHTValues(float h, float t, float hic) {
  Serial.println();
  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.print(" %\t Temperature: ");
  Serial.print(t);
  Serial.print(" *C ");
  Serial.print("\t Heat index: ");
  Serial.print(hic);
  Serial.println(" *C ");
}

void loop() {
}
