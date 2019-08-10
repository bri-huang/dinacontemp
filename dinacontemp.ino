/********************************************************************************/
// dinaconTemp.ino
// Temp / Humidity Sensor Reporting
//
// Hardware:
//    * RTU03 Temp / Humidity Sensor
//    * WS2812 LED Strand (x2)
//    * Serial LCD
//    * ESP8266 Thing
/********************************************************************************/

#include <SparkFun_RHT03.h>
#include <CayenneMQTTESP8266.h>
#include <Wire.h>
#include <SFE_BMP180.h>
#include <SerLCD.h>
#include <Adafruit_NeoPixel.h>

#define LCD_I2C   0x72
#define ALTITUDE  51
#define LED_PIN   13
#define LED_PIN2   12

#define LED_COUNT 30

RHT03 rht; // This creates a RTH03 object, which we'll use to interact with the sensor
SFE_BMP180 pressure;
SerLCD lcd;
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip2(LED_COUNT, LED_PIN2, NEO_GRB + NEO_KHZ800);

int ndx;
// WiFi network info.
char ssid[] = "DINALAB";
char buff = 0;
char wifiPassword[] = "iloveyardpigs";

// Cayenne authentication info. This should be obtained from the Cayenne Dashboard.
char username[] = "5bc5e000-bb16-11e9-80af-177b80d8d7b2";
char password[] = "9a0b1e42cd0f0f4bc136cf2e6171fa42b42030de";
char clientID[] = "63133ab0-bb16-11e9-b6c9-25dbdbf93e02";
// Cayenne dashboard: https://cayenne.mydevices.com/cayenne/dashboard/esp8266/0d923e90-b7c1-11e9-b6c9-25dbdbf93e02

double BMPTempC, BMPTempF, BMPPressure, P0, a;
long status;
unsigned long data_ndx;

float latestHumidity;
float latestTempC;
float latestTempF;

float minTemp = 80;
float maxTemp = 90;

/********************************************************************************/
void setup() {
  pinMode(5, OUTPUT);
  rht.begin(4);
  Wire.begin();
  lcd.begin(Wire); //Set up the LCD for I2C communication

  lcd.clear(); //Clear the display - this moves the cursor to home position as well

  lcd.print("Connecting to:  ");
  lcd.print(ssid);

  //  setupBMP();
  Cayenne.begin(username, password, clientID, ssid, wifiPassword);
  digitalWrite(5, HIGH);

  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(50); // Set BRIGHTNESS to about 1/5 (max = 255)
  colorWipe(strip.Color(255,   0,   0), 50); // Red
  colorWipe(strip.Color(0,   255,   0), 50); // Green
  colorWipe(strip.Color(0,   0,   255), 50); // Blue
  theaterChaseRainbow(50);
}
/********************************************************************************/
void loop()
{
  int updateRet = rht.update();
  //  readBMP();
  if (updateRet == 1)
  {
    latestHumidity = rht.humidity();
    latestTempC = rht.tempC();
    latestTempF = rht.tempF();
    ndx++;

    //    digitalWrite(5, HIGH);
    //    delay(200);
    //    digitalWrite(5, LOW);
    //    delay(200);

    int ledIndex = map(latestTempF, minTemp, maxTemp, 0, LED_COUNT);
    setPixels(LED_COUNT, 0, 0, 0);
    setPixels(ledIndex, 38, 0, 7);

    //ledCounter();

    // Now print the values:
    lcd.clear();
    lcd.print("Temp: ");
    lcd.print(latestTempF);
    lcd.print("F ");
    lcd.setCursor(0, 1);
    lcd.print("Hum: ");
    lcd.print(latestHumidity);
    lcd.print("%");
    Cayenne.loop();
  }
}

/********************************************************************************/
void ledCounter()
{
  if ((ndx % LED_COUNT) == 0)
  {
    setPixels(LED_COUNT, 0, 0, 0);
  }
  else
  {
    setPixels(ndx % LED_COUNT, random(255), random(255), random(255));
  }
}

/********************************************************************************/
void setPixels(unsigned int num, byte red, byte green, byte blue)
{
  for (int i = 0; i < num; i++) { // For each pixel...
    strip.setPixelColor(i, strip.Color(red, green, blue));
  }
  strip.show();   // Send the updated pixel colors to the hardware.
}
/********************************************************************************/
void colorWipe(uint32_t color, int wait) {
  for (int i = 0; i < strip.numPixels(); i++) { // For each pixel in strip...
    strip.setPixelColor(i, color);         //  Set pixel's color (in RAM)
    strip.show();                          //  Update strip to match
    delay(wait);                           //  Pause for a moment
  }
}
/********************************************************************************/
void readBMP()
{
  status = pressure.startTemperature();
  if (status != 0)
  {
    // Wait for the measurement to complete:
    delay(status);
    status = pressure.getTemperature(BMPTempC);
    BMPTempF = (9.0 / 5.0) * BMPTempC + 32.0;
    if (status != 0)
    {
      status = pressure.startPressure(3);
      if (status != 0)
      {
        // Wait for the measurement to complete:
        delay(status);
        status = pressure.getPressure(BMPPressure, BMPTempC);
        if (status != 0)
        {
          P0 = pressure.sealevel(BMPPressure, ALTITUDE); // we're at 1655 meters (Boulder, CO)
          a = pressure.altitude(BMPPressure, P0);
        }
        else; //Serial.println("error retrieving pressure measurement\n");
      }
      else; //Serial.println("error starting pressure measurement\n");
    }
    else; //Serial.println("error retrieving temperature measurement\n");
  }
  else; //Serial.println("error starting temperature measurement\n");

  clearLCD();
  delay(1);
  LCDWrite("Status: ");
  LCDWrite(status);
  delay(1);
  cursorPos(1, 0);
  delay(1);
  LCDWrite("Pressure: ");
  LCDWrite(P0);
}
/////////////////////////////////////////////////////////////////
void setupI2C()
{
  Wire.beginTransmission(LCD_I2C);
  Wire.write(0x7C);
  Wire.write(0x30);  // disable splash screen

  Wire.write(0x7C);
  Wire.write(0x2F);

  Wire.write(0x7C);
  Wire.write(0x04);
  Wire.write(0x7C);
  Wire.write(0x06);

  // set BackLight on
  Wire.write(0x7C);
  Wire.write(0x2B);
  Wire.write(0xFF);  // red
  Wire.write(0xFF);  // green
  Wire.write(0xFF);  // blue
  Wire.endTransmission();

  Wire.beginTransmission(LCD_I2C);
  Wire.write("DINALAB ENVIR   ");
  Wire.write("SENSOR VIZ");

  Wire.write(0x7C);
  Wire.write(0x0A);
  Wire.endTransmission();

}
/******************************************************************/
void LCDWrite(char *s)
{
  Wire.beginTransmission(LCD_I2C);
  Wire.endTransmission();
}
/******************************************************************/
void LCDWrite(char *s, int len)
{
  Wire.beginTransmission(LCD_I2C);
  Wire.write(s, len - 1);
  Wire.endTransmission();
}
/******************************************************************/
void LCDWrite(float num)
{
  Wire.beginTransmission(LCD_I2C);
  Wire.print(num);
  Wire.endTransmission();
}
/******************************************************************/
void clearLCD()
{
  Wire.beginTransmission(LCD_I2C);
  Wire.write(0x7C);
  Wire.write(0x2D);
  Wire.endTransmission();
}
/******************************************************************/
void theaterChaseRainbow(int wait) {
  int firstPixelHue = 0;     // First pixel starts at red (hue 0)
  for (int a = 0; a < 30; a++) { // Repeat 30 times...
    for (int b = 0; b < 3; b++) { //  'b' counts from 0 to 2...
      strip.clear();         //   Set all pixels in RAM to 0 (off)
      for (int c = b; c < strip.numPixels(); c += 3) {
        int      hue   = firstPixelHue + c * 65536L / strip.numPixels();
        uint32_t color = strip.gamma32(strip.ColorHSV(hue)); // hue -> RGB
        strip.setPixelColor(c, color); // Set pixel 'c' to value 'color'
      }
      strip.show();                // Update strip with new contents
      delay(wait);                 // Pause for a moment
      firstPixelHue += 65536 / 90; // One cycle of color wheel over 90 frames
    }
  }
}
/******************************************************************/
void setupBMP()
{
  if (pressure.begin())
  {
    Serial.println("BMP180 init success");
    digitalWrite(5, HIGH);
    delay(20);
    digitalWrite(5, LOW);
    delay(20);
    digitalWrite(5, HIGH);
    delay(20);
    digitalWrite(5, LOW);
    delay(20);
  }
  else
  {
    Serial.println("BMP180 init fail\n\n");
    while (1)
    {
      digitalWrite(5, HIGH);
      delay(100);
      digitalWrite(5, LOW);
      delay(100);
    }
  }
  digitalWrite(5, LOW);
}

/******************************************************************/
void cursorPos(int row, int col)
{
  byte cursorPos;
  cursorPos = 0x80 + (0x40) * row + col;
  Wire.beginTransmission(LCD_I2C);
  Wire.write(0xFE);
  Wire.write(0xC0);  // 2nd line
  Wire.endTransmission();
}

/******************************************************************/
CAYENNE_OUT_DEFAULT()
{
  // Write data to Cayenne here. This example just sends the current uptime in milliseconds on virtual channel 0.
  Cayenne.virtualWrite(0, data_ndx);
  Cayenne.virtualWrite(1, latestTempF);
  Cayenne.virtualWrite(2, latestHumidity);
  //  Cayenne.virtualWrite(3, BMPTempF);
  //  Cayenne.virtualWrite(4, BMPPressure);
  data_ndx++;
}

// Default function for processing actuator commands from the Cayenne Dashboard.
// You can also use functions for specific channels, e.g CAYENNE_IN(1) for channel 1 commands.
CAYENNE_IN_DEFAULT()
{
  CAYENNE_LOG("Channel %u, value %s", request.channel, getValue.asString());
  //Process message here. If there is an error set an error message using getValue.setError(), e.g getValue.setError("Error message");
}
