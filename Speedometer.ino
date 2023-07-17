#include <HardwareSerial.h>
#include <TinyGPSPlus.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Timezone.h>

// Einstellungen für UART zum GPS-Sensor
int RXPin = 20, TXPin = 21;
int GPSBaudrate = 115200;
HardwareSerial gpsSerial(1);

TinyGPSPlus gps;

//Central European Time (Frankfurt, Paris)  120 = +2 hours in daylight saving time (summer).
TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120};
//Central European Time (Frankfurt, Paris)  60  = +1 hour in normal time (winter)
TimeChangeRule CET = {"CET ", Last, Sun, Oct, 3, 60};
Timezone CE(CEST, CET);
time_t local;

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);




void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  gpsSerial.begin(GPSBaudrate, SERIAL_8N1, RXPin, TXPin);
  if (!gpsSerial) {  // If the object did not initialize, then its configuration is invalid
    Serial.println("Invalid EspSoftwareSerial pin configuration, check config");
    while (1) {  // Don't continue with invalid configuration
      delay(1000);
    }
  }

  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Clear the buffer
  display.clearDisplay();
}



void loop() {
  // This sketch displays information every time a new sentence is correctly encoded.#
  while (gpsSerial.available() > 0){
    if (gps.encode(gpsSerial.read())) {
      GPS_Timezone_Adjust();
      //displayInfo();
      outputToDisplay();
    }
  }
}

/*
 * GPS_TIMEZONE_ADJUST() - Umrechnung der GPS-Zeit von UTC zu CEST
 */

void GPS_Timezone_Adjust(){
  int Year = gps.date.year();
  byte Month = gps.date.month();
  byte Day = gps.date.day();
  byte Hour = gps.time.hour();
  byte Minute = gps.time.minute();
  byte Second = gps.time.second();

  // Set Time from GPS data string
  setTime(Hour, Minute, Second, Day, Month, Year);
  // Calc current Time Zone time by offset value
  local = CE.toLocal(now());      
  
}

/*
 * Spaceinvader Symbol für die Anzahl der Satelliten
 */

// 'gps', 11x8px
const unsigned char gpsBitmap [] PROGMEM = {
	0x20, 0x80, 0x11, 0x00, 0x3f, 0x80, 0x6e, 0xc0, 0xff, 0xe0, 0xbf, 0xa0, 0xa0, 0xa0, 0x1b, 0x00
};

/*
 * OUTPUTTODISPLAY() - Ausgabe der GPS-Daten auf das SSD1306-Display
 */

void outputToDisplay() {
  display.clearDisplay();
  display.drawLine(0, 10, 128, 10, WHITE);
  display.drawLine(0, 54, 128, 54, WHITE);

  display.setTextSize(1);
  display.setTextColor(WHITE);

  display.setCursor(1, 1);
  display.print("HDOP: ");
  display.print((float)gps.hdop.value()/100);

  /* // Debugausgeben zum betrachten der bearbeiteten NMEA-Sätze
  display.setCursor(1, 12);
  display.print(gps.sentencesWithFix());
  display.setCursor(1, 22);
  display.print(gps.charsProcessed());
  */
  

  int satellites = (int)gps.satellites.value();
  if (satellites < 10) {
    display.setCursor(121, 1);
    display.drawBitmap(108, 0, gpsBitmap, 11, 8, WHITE);
  } else if (satellites < 100) {
    display.setCursor(114, 1);
    display.drawBitmap(101, 0, gpsBitmap, 11, 8, WHITE);
  } else {
    display.setCursor(107, 1);
    display.drawBitmap(94, 0, gpsBitmap, 11, 8, WHITE);
  }
  display.print(satellites);

  display.setCursor(5, 56);

  if (gps.date.isValid() && gps.time.isValid()) {
    if (day(local) < 10) display.print("0");
    display.print(day(local));
    display.print(F("."));
    if (month(local) < 10) display.print("0");
    display.print(month(local));
    display.print(F("."));
    display.print(year(local));

    display.print(F(" "));

    if (hour(local) < 10) display.print("0");
    display.print(hour(local));
    display.print(":");
    if (minute(local) < 10) display.print("0");
    display.print(minute(local));
    display.print(":");
    if (second(local) < 10) display.print("0");
    display.print(second(local));
  } else {
    display.print(F("--.--.---- --:--:--"));
  }

  display.setTextSize(4);
  display.setTextColor(WHITE);
  if (gps.speed.isValid()) {
    //int speed = random(0, 35);
    int speed = gps.speed.kmph();
    if ((int)speed < 10) display.setCursor(74, 20);
    else if ((int)speed < 100) display.setCursor(50, 20);
    else display.setCursor(26, 20);

    display.print(speed, 0);
  } else {
    display.setCursor(26, 20);
    display.print("---");
  }
  
  display.setTextSize(1);
  display.setCursor(98, 32);
  display.print(" km/h");


  display.display();
}



/*
 *  DISPLAYINFO() - Ausgabe wichtiger GPS Daten über UART an die Arduino IDE
 */


void displayInfo() {
  Serial.print(F("Speed: "));
  if (gps.speed.isValid()) {
    Serial.print(gps.speed.kmph(), 6);
  } else {
    Serial.print(F("INVALID"));
  }
  Serial.print(F(" "));
  
  Serial.print(F("Satellites: "));
  if (gps.satellites.isValid())
  {
    Serial.print(gps.satellites.value());
  } else {
    Serial.print(F("INVALID"));
  }
  Serial.print(F(" "));
  Serial.print(F("Location: "));
  if (gps.location.isValid()) {
    Serial.print(gps.location.lat(), 6);
    Serial.print(F(","));
    Serial.print(gps.location.lng(), 6);
  } else {
    Serial.print(F("INVALID"));
  }

  Serial.print(F("  Date/Time: "));
  if (gps.date.isValid()) {
    Serial.print(gps.date.month());
    Serial.print(F("/"));
    Serial.print(gps.date.day());
    Serial.print(F("/"));
    Serial.print(gps.date.year());
  } else {
    Serial.print(F("INVALID"));
  }

  Serial.print(F(" "));
  if (gps.time.isValid()) {
    if (gps.time.hour() < 10) Serial.print(F("0"));
    Serial.print(gps.time.hour());
    Serial.print(F(":"));
    if (gps.time.minute() < 10) Serial.print(F("0"));
    Serial.print(gps.time.minute());
    Serial.print(F(":"));
    if (gps.time.second() < 10) Serial.print(F("0"));
    Serial.print(gps.time.second());
    Serial.print(F("."));
    if (gps.time.centisecond() < 10) Serial.print(F("0"));
    Serial.print(gps.time.centisecond());
  } else {
    Serial.print(F("INVALID"));
  }

  Serial.println();
}