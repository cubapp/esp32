

// Include the correct display library
// For a connection via I2C using Wire include
#include <Wire.h>  // Only needed for Arduino 1.6.5 and earlier
#include "SSD1306.h" // alias for `#include "SSD1306Wire.h"`
// or #include "SH1106.h" alis for `#include "SH1106Wire.h"`

// Include custom images
#include "images.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <esp_system.h>
#include "DHT.h"
 
#define DHTPIN 26
//our sensor is AM230x type
// other are in DHT.h 
#define DHTTYPE AM2301

//create an instance of DHT sensor
DHT dht(DHTPIN, DHTTYPE);

const char* ssid = "mySSIDwifi";
const char* password = "OpenWifiGates42";
const int wdtTimeout = 10000;  //time in ms to trigger the watchdog
hw_timer_t *timer = NULL;



// Initialize the OLED display using Wire library
SSD1306  display(0x3c, 5, 4);
// SH1106 display(0x3c, D3, D5);


#define DEMO_DURATION 3000
typedef void (*Demo)(void);

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  3        /* Time ESP32 will go to sleep (in seconds) */


int demoMode = 0;
int counter = 1;
float h=0;
float t=0;

String payload="No Payload";

static bool go_to_sleep(void)
{
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
    esp_deep_sleep_start();
}

void IRAM_ATTR resetModule() {
  Serial.println("ResetModule now..");
  ets_printf("reboot\n");
  esp_restart();
}

 
void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();
  WiFi.begin(ssid, password);
  dht.begin();
 
  int badwifi = 0;
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
    badwifi++;
    if (badwifi>100){
      go_to_sleep();
    }
  }
 
  Serial.println("Connected to the WiFi network");
  
  // Initialising the UI will init the display too.
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);

  timer = timerBegin(0, 80, true);                  //timer 0, div 80
  timerAttachInterrupt(timer, &resetModule, true);  //attach callback
  timerAlarmWrite(timer, wdtTimeout * 1000, false); //set time in us
  timerAlarmEnable(timer); //enable interrupt
  counter = 0;

}



String getHTTPdata(void){
  String payload="No Payload";
  HTTPClient http;
  http.setTimeout(3000);
  http.begin("http://www.data.somewherree.com/data.txt"); //Specify the URL
  int httpCode = http.GET();  
  if (httpCode > 0) { //Check for the returning code
        payload = http.getString();
        Serial.println(httpCode);
        Serial.println(payload);
  }else {
      Serial.println("Error on HTTP request");
      payload = "Error in HTTP request";
  }
  http.end(); //Free the resources
  return (payload);
}


void displayTemp() {
    // Font Demo1
    // create more fonts at http://oleddisplay.squix.ch/
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_24);
    display.drawString(0, 0, String(t)+" °C");
    display.drawString(0, 24,String(h)+" % ");
    display.setFont(ArialMT_Plain_10);
    display.drawString(50, 49, String(counter));
      
}

void httpGetNewTemp() {
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    delay(1000);
    display.drawStringMaxWidth(0, 0, 128,
    "Getting new data with counter: "+String(counter));
    payload = getHTTPdata();

/* plus get DHT temp and humidity */

 h = dht.readHumidity();
// Read temperature as Celsius (the default)
 t = dht.readTemperature();
// Check if any reads failed and exit early (to try again).

if (isnan(h) || isnan(t)) {
Serial.println("Failed to read from DHT sensor!");
return;
}
// print the result to Terminal
Serial.print("Humidity: ");
Serial.print(h);
Serial.print(" %\t");
Serial.print("Temperature: ");
Serial.print(t);
Serial.println(" *C ");
//we delay a little bit for next read

    
    
}

Demo demos[] = { httpGetNewTemp, displayTemp };

void loop() {
  // clear the display
  
  display.clear();
  Serial.println("Demos started with counter: " + String(counter));
  timerWrite(timer, 0);
  demos[demoMode]();
  
  display.setTextAlignment(TEXT_ALIGN_RIGHT);
  display.drawString(0, 18, String(millis()));
  // write the buffer to the display
  display.display();
   

  counter++;
  delay(100);
  if ( !(counter % 100))  {
    // call httpGetNewTemp, get new HTTP 
    demoMode = 0;
  }
  else {
    // standard screening of the data 
    demoMode = 1; 
  }




  
  if (counter > 2000){
     display.clear();
     Serial.println("Restarting in 10 seconds! Counter is: ");
     Serial.println(counter);
     
     display.drawString(1, 18, String(counter));
     display.drawString(100, 24, "Restarting");
     // write the buffer to the display
     display.display();
     delay(1000);
     display.drawString(124, 24, "Restart!!!");
     display.display();
     Serial.println("Restart!!!");
     delay(500);
     counter=0;
     go_to_sleep();
  }
}

