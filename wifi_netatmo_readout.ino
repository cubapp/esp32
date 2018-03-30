#include <Wire.h>  // Only needed for Arduino 1.6.5 and earlier
#include "SSD1306.h" // alias for `#include "SSD1306Wire.h"`
// or #include "SH1106.h" alis for `#include "SH1106Wire.h"`
// For a connection via I2C using brzo_i2c (must be installed) include
// #include <brzo_i2c.h> // Only needed for Arduino 1.6.5 and earlier
// #include "SSD1306Brzo.h"
// #include "SH1106Brzo.h"
// For a connection via SPI include
// #include <SPI.h> // Only needed for Arduino 1.6.5 and earlier
// #include "SSD1306Spi.h"
// #include "SH1106SPi.h"

// Include custom images
//#include "images.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <esp_system.h>
 
// SSID and Password of your network WiFi
const char* ssid = "MinisterstvoVnitra";
const char* password = "ProdamCeckaZnLevne";

// Initialize the OLED display using SPI
// D5 -> CLK
// D7 -> MOSI (DOUT)
// D0 -> RES
// D2 -> DC
// D8 -> CS
// SSD1306Spi        display(D0, D2, D8);
// or
// SH1106Spi         display(D0, D2);

// Initialize the OLED display using brzo_i2c
// D3 -> SDA
// D5 -> SCL
// SSD1306Brzo display(0x3c, D3, D5);
// or
// SH1106Brzo  display(0x3c, D3, D5);

// Initialize the OLED display using Wire library
SSD1306  display(0x3c, 5, 4);
// SH1106 display(0x3c, D3, D5);


#define DEMO_DURATION 3000
typedef void (*Demo)(void);

int demoMode = 0;
int counter = 1;

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();

   WiFi.begin(ssid, password);
  String payload="No Payload";
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
 
  Serial.println("Connected to the WiFi network");
  
  // Initialising the UI will init the display too.
  display.init();

  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);

}

String getHTTPdata(void){
  String payload="No Payload";
  HTTPClient http;
  http.begin("http://www.cnn.com/news.txt"); //Specify the URL
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


void drawFontFaceDemo() {
    // Font Demo1
    // create more fonts at http://oleddisplay.squix.ch/
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_16);
    display.drawString(0, 0, "Teploty:____"+String(counter));
    display.setFont(ArialMT_Plain_10);
    display.drawString(0, 18, getHTTPdata());
      
}



void drawTextFlowDemo() {
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.drawStringMaxWidth(0, 0, 128,
    "Nacitame dalsi teplotu, asi se ale moc nezmenila..." );
    delay(19000);
}

Demo demos[] = {drawFontFaceDemo, drawTextFlowDemo };
int demoLength = (sizeof(demos) / sizeof(Demo));
long timeSinceLastModeSwitch = 0;

void loop() {
  // clear the display
  display.clear();
  // draw the current demo method
  demos[demoMode]();

  display.setTextAlignment(TEXT_ALIGN_RIGHT);
  display.drawString(0, 18, String(millis()));
  // write the buffer to the display
  display.display();

  if (millis() - timeSinceLastModeSwitch > DEMO_DURATION) {
    demoMode = (demoMode + 1)  % demoLength;
    timeSinceLastModeSwitch = millis();
  }
  counter++;
  delay(2000);
  if (counter > 40){
     display.clear();
     Serial.println("Restarting in 10 seconds! Counter is: ");
     Serial.println(counter);
     
     display.drawString(1, 18, String(counter));
     display.drawString(1, 24, "Restarting");
     // write the buffer to the display
     display.display();
     delay(1000);
     display.drawString(12, 24, "Restart!!!");
     display.display();
     Serial.println("Restart!!!");
     delay(500);
     
     esp_restart();
  }
}

