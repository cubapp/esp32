/* 
 * An ESP32 and 1.3" OLED WiFi Signal Strength Site Survey Tool/Utility, As you move around you can determine signal strength of a WiFi Router or Extender, which is useful
 * for determining where poor Wi-Fi reception is going to be or is being encountered.
 * 
 * The 'MIT License (MIT) Copyright (c) 2017 by David Bird'. Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated 
 * documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish,  
 * distribute, sublicense, and/or sell copies of the Software and to permit persons to whom the Software is furnished to do so, subject to the following conditions:    
 * The above copyright ('as annotated') notice and this permission notice shall be included in all copies or substantial portions of the Software and where the   
 * software use is visible to an end-user.  
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHOR OR COPYRIGHT HOLDER BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER    
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * See more at http://dsbird.org.uk
*/
// from https://github.com/G6EJD/ESP32-Site-Survey-Tool


#include "WiFi.h"
#include "SSD1306.h" 
SSD1306  display(0x3c, 5, 4);


#define STARS 100

float star_x[STARS], star_y[STARS], star_z[STARS]; 

void initStar(int i) {
  star_x[i] = random(-100, 100);
  star_y[i] = random(-100, 100);
  star_z[i] = random(100, 500);
}

void showStarfield() {
  int x,y;
  int centrex,centrey;
  
  centrex = DISPLAY_WIDTH / 2;
  centrey = (DISPLAY_HEIGHT / 4 ) + 32 ; 
  
  for (int i = 0; i < STARS; i++) {
    star_z[i] = star_z[i] - 7;

    x = star_x[i] / star_z[i] * 100 + centrex;
    y = star_y[i] / star_z[i] * 100 + centrey;

    if(
        (x < 0)  ||        (x > DISPLAY_WIDTH) || 
        (y < 0)  ||        (y > DISPLAY_HEIGHT) ||
        (star_z[i] < 1)      
      )      initStar(i);
    display.setPixel(x, y);
  }
  display.display();
} 




void setup(){
    Serial.begin(115200);               // For serial diagnostic prints
    WiFi.mode(WIFI_STA);                // Put ESP32 into Station Mode
    Wire.begin(5,4); // (sda,scl)       // Start the Wire service for the OLED display using pin=4 for SCL and Pin-5 for SDA
    display.init();                     // Initialise the display  
    display.flipScreenVertically();     // In my case flip the screen around by 180°
    display.setContrast(10);           // If you want turn the display contrast down, 255 is maxium and 0 in minimum, in practice about 128 is OK
    display.setFont(ArialMT_Plain_10);  // Set the Font size
    for (int i = 0; i < STARS; i++) 
        initStar(i);
}

void loop(){
    byte available_networks = WiFi.scanNetworks(); // Count the number of networks being received
    if (available_networks == 0) {
        Serial.println("no networks found");
    } else
    {
      display.clear();
      display.drawString(0,0,"SSID");display.drawString(96,0,"Signal");display.drawLine(0,11,127,11);
      display.drawLine(0,0,0,91);
      display.drawLine(0,0,127,0);
      display.drawLine(127,0,127,91);
      display.drawLine(0,63,127,63);
      display.display();
      for (int i = 1; i < available_networks; i = i + 1) {
        display.drawString(0,i*9+1,WiFi.SSID(i)+" ");       // Display SSID name
        display.drawString(110,i*9+1,String(WiFi.RSSI(i))); // Display RSSI for that network
        
        display.display();
      }
    }
    display.display();
    delay(1500); // Wait before scanning again
    
    showStarfield();
    
    delay(500);
    
}
