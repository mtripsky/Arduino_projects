#include <Wire.h>
#include <LiquidCrystal_I2C.h>  

LiquidCrystal_I2C lcd(0x3F,16,2); // set the LCD address to 0x3F for a 16 chars and 2 line display

// Moisture sensor
const int AirValue = 670;
const int WaterValue = 250;
int soilMoistureValue = 0;
int soilmoisturepercent = 0;

// Time to wait before measure again (in seconds):
const int waitTimeS = 30;

void setup() {
  Serial.begin(115200); // open serial port, set the baud rate to 9600 bps

  lcd.init(); // initialize the lcd
  //lcd.init();
  // Print a message to the LCD.
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("Soil noisture:");
  lcd.setCursor(0,1);
  lcd.print("Sensor starting ....");
  delay(1000);
}

void printResults(float moistureValue, float moisturePercentage)
{
  Serial.println();
  Serial.print("Moisture: ");
  Serial.print(moistureValue);
  Serial.print(" V\t Moisture: ");
  Serial.print(moisturePercentage);
  Serial.print(" % ");
}

void loop() {
  lcd.clear();
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

  printResults(soilMoistureValue, soilmoisturepercent);

  lcd.setCursor(0,0);
  lcd.print("Soil noisture:");
  lcd.setCursor(2,1);
  lcd.print(soilmoisturepercent);
  lcd.setCursor(5,1);
  lcd.print("%");
  lcd.setCursor(10,1);
  lcd.print(soilMoistureValue);
  lcd.setCursor(14,1);
  lcd.print("V");
  delay(waitTimeS * 1000);
}
