#define USE_ARDUINO_INTERRUPTS false
#include <PulseSensorPlayground.h>

#include <ESPAsyncWebServer.h>
#include <WiFI.h>
#include <ESPmDNS.h>
#include <AsyncJson.h>
#include <SPIFFS.h>

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>


#define i2c_Address 0x3c 



double x, y;

bool Redraw4 = true;
double ox , oy ;


#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1   //   QT-PY / XIAO
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

int counter = 0;
#define BUILTIN_LED 2
const int OUTPUT_TYPE = SERIAL_PLOTTER;

const int PULSE_INPUT = 36;
const int PULSE_BLINK = 2;    // Pin 13 is the on-board LED
const int PULSE_FADE = 23;
const int THRESHOLD = 3500;   // Adjust this number to avoid noise when idle

byte samplesUntilReport;
const byte SAMPLES_PER_SERIAL_SAMPLE = 10;

PulseSensorPlayground pulseSensor;
AsyncWebServer server(80);

void setup()
{

  
  Serial.begin(115200);

  delay(250); // wait for the OLED to power up
  display.begin(i2c_Address, true); // Address 0x3C default
 //display.setContrast (0); // dim display
  display.display();
  display.clearDisplay();
  display.display();
  /*
  for (int i=0; i<100; i++)
  {
  DrawCGraph(display, x++, random(0, 1024), 30, 50, 75, 30, 0, 100, 25, 0, 1024, 512, 0, "Place Holder for future graphs", Redraw4);
  }
  */

  
  pulseSensor.analogInput(PULSE_INPUT);
  pulseSensor.blinkOnPulse(PULSE_BLINK);
  pulseSensor.fadeOnPulse(PULSE_FADE);

  pulseSensor.setSerial(Serial);
  pulseSensor.setOutputType(OUTPUT_TYPE);
  pulseSensor.setThreshold(THRESHOLD);

  // Skip the first SAMPLES_PER_SERIAL_SAMPLE in the loop().
  samplesUntilReport = SAMPLES_PER_SERIAL_SAMPLE;

  // Now that everything is ready, start reading the PulseSensor signal.
  if (!pulseSensor.begin()) {
    /*
       PulseSensor initialization failed,
       likely because our Arduino platform interrupts
       aren't supported yet.

       If your Sketch hangs here, try changing USE_PS_INTERRUPT to false.
    */
    for(;;) {
      // Flash the led to show things didn't work.
      digitalWrite(PULSE_BLINK, LOW);
      delay(50);
      digitalWrite(PULSE_BLINK, HIGH);
      delay(50);
    }
  }
   delay(10);

    // We start by connecting to a WiFi network

    Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println("AndroidAPB397");

    WiFi.begin("AndroidAPB397", "wlmp1425");
    

    while (WiFi.status() != WL_CONNECTED) {
        counter++;
        delay(1000);
        Serial.print(".");
        if (counter >= 60)
        {
          Serial.print("\nTrying again by restarting the esp\n");
          ESP.restart();
        }
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

  Serial.println();


  SPIFFS.begin();

  MDNS.begin("demo-server");

 

  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, PUT");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "*");

  /* server.addHandler(new AsyncCallbackJsonWebHandler("/led", [](AsyncWebServerRequest *request, JsonVariant &json) {
    const JsonObject &jsonObj = json.as<JsonObject>();
    if (jsonObj["on"])
    {
      Serial.println("Turn on LED");
      digitalWrite(BUILTIN_LED, HIGH);
    }
    else
    {
      Serial.println("Turn off LED");
      digitalWrite(BUILTIN_LED, LOW);
    }
    request->send(200, "OK");
  }));*/


  
  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

  server.on("/IBI", HTTP_GET, [](AsyncWebServerRequest *request){
  request->send_P(200, "text/plain", String(pulseSensor.getInterBeatIntervalMs()).c_str());
  });

  server.on("/BPM", HTTP_GET, [](AsyncWebServerRequest *request){
  request->send_P(200, "text/plain", String(pulseSensor.getBeatsPerMinute() / 2).c_str());
  });

  server.onNotFound([](AsyncWebServerRequest *request) {
    if (request->method() == HTTP_OPTIONS)
    {
      request->send(200);
    }
    else
    {
      Serial.println("Not found");
      request->send(404, "Not found");
    }
  });

  server.begin();
}

void loop()
{
  
    if (pulseSensor.sawNewSample()) {
    /*
       Every so often, send the latest Sample.
       We don't print every sample, because our baud rate
       won't support that much I/O.
    */
    if (--samplesUntilReport == (byte) 0) {
      samplesUntilReport = SAMPLES_PER_SERIAL_SAMPLE;

      pulseSensor.outputSample();
      if ( x < 100)
  {
    DrawCGraph(display, x++,pulseSensor.getBeatsPerMinute() , 30, 50, 75, 30, 0, 100, 25, 0, 1024, 512, 0, "Place Holder for future graphs", Redraw4);
  }

      /*
         At about the beginning of every heartbeat,
         report the heart rate and inter-beat-interval.
      */
      if (pulseSensor.sawStartOfBeat()) {
        pulseSensor.outputBeat();
      }
    }

    /*******
      Here is a good place to add code that could take up
      to a millisecond or so to run.
    *******/
  }
}



void DrawCGraph(Adafruit_SH1106G &d, double x, double y, double gx, double gy, double w, double h, double xlo, double xhi, double xinc, double ylo, double yhi, double yinc, double dig, String title, boolean &Redraw) {

  double i;
  double temp;
  int rot, newrot;

  if (Redraw == true) {
    Redraw = false;
    d.fillRect(0, 0,  127 , 16, SH110X_WHITE);
    d.setTextColor(SH110X_BLACK, SH110X_WHITE);
    d.setTextSize(1);
    d.setCursor(2, 4);
    d.println(title);
    ox = (x - xlo) * ( w) / (xhi - xlo) + gx;
    oy = (y - ylo) * (gy - h - gy) / (yhi - ylo) + gy;
    // draw y scale
    d.setTextSize(1);
    d.setTextColor(SH110X_WHITE, SH110X_BLACK);
    for ( i = ylo; i <= yhi; i += yinc) {
      // compute the transform
      // note my transform funcition is the same as the map function, except the map uses long and we need doubles
      temp =  (i - ylo) * (gy - h - gy) / (yhi - ylo) + gy;
      if (i == 0) {
        d.drawFastHLine(gx - 3, temp, w + 3, SH110X_WHITE);
      }
      else {
        d.drawFastHLine(gx - 3, temp, 3, SH110X_WHITE);
      }
      d.setCursor(gx - 27, temp - 3);
      d.println(i, dig);
    }
    // draw x scale
    for (i = xlo; i <= xhi; i += xinc) {
      // compute the transform
      d.setTextSize(1);
      d.setTextColor(SH110X_WHITE, SH110X_BLACK);
      temp =  (i - xlo) * ( w) / (xhi - xlo) + gx;
      if (i == 0) {
        d.drawFastVLine(temp, gy - h, h + 3, SH110X_WHITE);
      }
      else {
        d.drawFastVLine(temp, gy, 3, SH110X_WHITE);
      }
      d.setCursor(temp, gy + 6);
      d.println(i, dig);
    }
  }

  // graph drawn now plot the data
  // the entire plotting code are these few lines...

  x =  (x - xlo) * ( w) / (xhi - xlo) + gx;
  y =  (y - ylo) * (gy - h - gy) / (yhi - ylo) + gy;
  d.drawLine(ox, oy, x, y, SH110X_WHITE);
  d.drawLine(ox, oy - 1, x, y - 1, SH110X_WHITE);
  ox = x;
  oy = y;

  // up until now print sends data to a video buffer NOT the screen
  // this call sends the data to the screen
  d.display();
  
}
