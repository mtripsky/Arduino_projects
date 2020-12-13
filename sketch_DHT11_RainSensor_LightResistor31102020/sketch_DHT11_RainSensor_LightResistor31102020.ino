#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "DHTesp.h"

// Initializes the espClient
WiFiClient espClient;
PubSubClient client(espClient);

// DHT Sensor
DHTesp dht;
const int DHTPin = 12; // D6

// Photoresistor
const int rainPin = 13;  // D7
const int photoresistorPin = 15; // D8
const int rainSensorNoRainValue = 928;
const int rainSensorRainValue = 0;
const int photoresistorHighIntensity = 820;
const int photoresistorLowIntensity = 0;
const char* ssid = "wifi-name";
const char* password = "password";

// Change the variable to your Raspberry Pi IP address, so it connects to your MQTT broker
const char* mqtt_server = "ip-address"; // Mac-book


// Time to sleep (in seconds):
const int sleepTimeS = 600;

WiFiServer server(80);

// JSON objects
StaticJsonBuffer<256> JSONbuffer;
JsonObject& JSONencoder = JSONbuffer.createObject();

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

    if (client.connect("ESP8266 Weather")) {
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
  dht.setup(DHTPin, DHTesp::DHT11);
  Serial.begin(115200); // open serial port, set the baud rate to 9600 bps
 // Serial.begin(9600); // open serial port, set the baud rate to 9600 bps
  setup_wifi();
  client.setServer(mqtt_server, 1883);

  //server.begin(); // Start the server

  pinMode(rainPin, OUTPUT);
  pinMode(photoresistorPin, OUTPUT);

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

  client.connect("ESP8266Client");

  JSONencoder["location"] = "OUTSIDE-WINDOW";
  runDHTsensor();
  runRainSensor();
  runPhotoresistorSensor();
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
}

void runRainSensor() {
  digitalWrite(rainPin, HIGH);
  delay(100);

  int value = analogRead(A0);
  int percentage = map(value, rainSensorNoRainValue, rainSensorRainValue, 0, 100);

  if(percentage > 100) {
    percentage = 100;
  }
  if(percentage < 0) {
    percentage = 0;
  }

  JSONencoder["device"] = "Rain-Droplet-Sensor";
  JSONencoder["type"] = "Rain_Intensity";
  JSONencoder["value"] = percentage;
  JSONencoder["unit"] = "%";

  char JSONmessageBufferIntensity[256];
  JSONencoder.printTo(JSONmessageBufferIntensity, sizeof(JSONmessageBufferIntensity));
  Serial.println(JSONmessageBufferIntensity);
  if(client.publish("/weather/rain_intensity/rain_sensor", JSONmessageBufferIntensity) == true)
  {
    Serial.println("Success sending rain sensor intensity message");
  } else {
    Serial.println("Error sending rain sensor intensity  message");
  }

  JSONencoder["type"] = "Rain_Intensity";
  JSONencoder["value"] = value;
  JSONencoder["unit"] = "V";

  char JSONmessageBufferVoltage[256];
  JSONencoder.printTo(JSONmessageBufferVoltage, sizeof(JSONmessageBufferVoltage));
  Serial.println(JSONmessageBufferVoltage);
  if(client.publish("/weather/rain_intensity/rain_sensor/voltage", JSONmessageBufferVoltage) == true)
  {
    Serial.println("Success sending rain sensor voltage message");
  } else {
    Serial.println("Error sending rain sensor voltage  message");
  }

  digitalWrite(rainPin, LOW);
  delay(100);
}

void runPhotoresistorSensor() {
  digitalWrite(photoresistorPin, HIGH);
  delay(100);

  if (!client.connected()) {
    reconnect();
  }
  if(!client.loop())
  client.connect("ESP8266Client");

  int value = analogRead(A0);
  int percentage = map(value, photoresistorLowIntensity, photoresistorHighIntensity, 0, 100);

  if(percentage > 100) {
    percentage = 100;
  }
  if(percentage < 0) {
    percentage = 0;
  }

  JSONencoder["device"] = "Photoresistor";

  JSONencoder["type"] = "Light_Intensity";
  JSONencoder["value"] = percentage;
  JSONencoder["unit"] = "%";

  char JSONmessageBufferIntensity[256];
  JSONencoder.printTo(JSONmessageBufferIntensity, sizeof(JSONmessageBufferIntensity));
  Serial.println(JSONmessageBufferIntensity);
  if(client.publish("/weather/light_intensity/photoresistor", JSONmessageBufferIntensity) == true)
  {
    Serial.println("Success sending light sensor intensity message");
  } else {
    Serial.println("Error sending light sensor intensity  message");
  }

  JSONencoder["type"] = "Light_Intensity";
  JSONencoder["value"] = value;
  JSONencoder["unit"] = "V";

  char JSONmessageBufferVoltage[256];
  JSONencoder.printTo(JSONmessageBufferVoltage, sizeof(JSONmessageBufferVoltage));
  Serial.println(JSONmessageBufferVoltage);
  if(client.publish("/weather/light_intensity/photoresistor/voltage", JSONmessageBufferVoltage) == true)
  {
    Serial.println("Success sending light sensor voltage message");
  } else {
    Serial.println("Error sending light sensor voltage  message");
  }

  digitalWrite(photoresistorPin, LOW);
  delay(100);
}

void printDHTValues(float h, float t, float hic) {
  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.print(" %\t Temperature: ");
  Serial.print(t);
  Serial.print(" *C ");
  Serial.print("\t Heat index: ");
  Serial.print(hic);
  Serial.println(" *C ");
}

void printRainSensor(float value, float percentage) {
  Serial.print("Rain Intensity value: ");
  Serial.print(value);
  Serial.print("\t");
  Serial.print("Rain Intensity: ");
  Serial.print(percentage);
  Serial.print(" %");
  Serial.println();
}

void printPhotoresistorSensor(float value, float percentage) {
  Serial.print("Photoresistor value: ");
  Serial.print(value);
  Serial.print("\t");
  Serial.print("Photoresistor intensity: ");
  Serial.print(percentage);
  Serial.print(" %");
  Serial.println();
}

void loop() {
}
