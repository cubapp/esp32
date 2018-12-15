/* uncomment if the default 4 bit mode doesn't work */
/* ------------------------------------------------ */
// #define BOARD_HAS_1BIT_SDMMC true // forces 1bit mode for SD MMC
/* ------------------------------------------------ */
// Taken from https://github.com/spacehuhn/PacketMonitor32
// 


#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"

#include <stdio.h>
#include <string>
#include <cstddef>
#include <Wire.h>
#include "SSD1306.h" 
SSD1306  display(0x3c, 5, 4);
#include <Preferences.h>
using namespace std;

/* ===== compile settings ===== */
#define MAX_CH 14       // 1 - 14 channels (1-11 for US, 1-13 for EU and 1-14 for Japan)
#define SNAP_LEN 2324   // max len of each recieved packet

#define BUTTON_PIN 0    // button to change the channel

#define USE_DISPLAY     // comment out if you don't want to use the OLED display
#define FLIP_DISPLAY    // comment out if you don't like to flip it
#define SDA_PIN 26
#define SCL_PIN 27
#define MAX_X 128
#define MAX_Y 51

#if CONFIG_FREERTOS_UNICORE
#define RUNNING_CORE 0
#else
#define RUNNING_CORE 1
#endif


esp_err_t event_handler(void* ctx, system_event_t* event) {
  return ESP_OK;
}

/* ===== run-time variables ===== */
Preferences preferences;

bool useSD = false;
bool buttonPressed = false;
bool buttonEnabled = true;
uint32_t lastDrawTime;
uint32_t lastButtonTime;
uint32_t tmpPacketCounter;
uint32_t pkts[MAX_X];       // here the packets per second will be saved
uint32_t deauths = 0;       // deauth frames per second
unsigned int ch = 11;        // current 802.11 channel
int rssiSum;

/* ===== functions ===== */
double getMultiplicator() {
  uint32_t maxVal = 1;
  for (int i = 0; i < MAX_X; i++) {
    if (pkts[i] > maxVal) maxVal = pkts[i];
  }
  if (maxVal > MAX_Y) return (double)MAX_Y / (double)maxVal;
  else return 1;
}

void setChannel(int newChannel) {
  ch = newChannel;
  if (ch > MAX_CH || ch < 1) ch = 1;

  preferences.begin("packetmonitor32", false);
  preferences.putUInt("channel", ch);
  preferences.end();

  esp_wifi_set_promiscuous(false);
  esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous_rx_cb(&wifi_promiscuous);
  esp_wifi_set_promiscuous(true);
}


void wifi_promiscuous(void* buf, wifi_promiscuous_pkt_type_t type) {
  wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
  wifi_pkt_rx_ctrl_t ctrl = (wifi_pkt_rx_ctrl_t)pkt->rx_ctrl;

  if (type == WIFI_PKT_MGMT && (pkt->payload[0] == 0xA0 || pkt->payload[0] == 0xC0 )) deauths++;

  if (type == WIFI_PKT_MISC) return;             // wrong packet type
  if (ctrl.sig_len > SNAP_LEN) return;           // packet too long

  uint32_t packetLength = ctrl.sig_len;
  if (type == WIFI_PKT_MGMT) packetLength -= 4;  // fix for known bug in the IDF https://github.com/espressif/esp-idf/issues/886

  //Serial.print(".");
  tmpPacketCounter++;
  rssiSum += ctrl.rssi;

}

void draw() {
#ifdef USE_DISPLAY
  double multiplicator = getMultiplicator();
  int len;
  int rssi;

  if (pkts[MAX_X - 1] > 0) rssi = rssiSum / (int)pkts[MAX_X - 1];
  else rssi = rssiSum;

  display.clear();

  display.setTextAlignment(TEXT_ALIGN_RIGHT);
  display.drawString( 10, 0, (String)ch);
  display.drawString( 14, 0, ("|"));
  display.drawString( 30, 0, (String)rssi);
  display.drawString( 34, 0, ("|"));
  display.drawString( 80, 0, (String)tmpPacketCounter);
  display.drawString( 84, 0, ("["));
  display.drawString(100, 0, (String)deauths);
  display.drawString(104, 0, ("]"));
  display.drawString(108, 0, ("|"));
  display.drawString(126, 0, (useSD ? "SD" : ""));
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString( 36,  0, ("Pkts:"));

  display.drawLine(0, 63 - MAX_Y, MAX_X, 63 - MAX_Y);
  for (int i = 0; i < MAX_X; i++) {
    len = pkts[i] * multiplicator;
    display.drawLine(i, 63, i, 63 - (len > MAX_Y ? MAX_Y : len));
    if (i < MAX_X - 1) pkts[i] = pkts[i + 1];
  }
  display.display();
#endif
}

/* ===== main program ===== */
void setup() {

  // Serial
  Serial.begin(115200);

  // Settings
  preferences.begin("packetmonitor32", false);
  ch = preferences.getUInt("channel", 11);
  preferences.end();

  // System & WiFi
  tcpip_adapter_init();
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  //ESP_ERROR_CHECK(esp_wifi_set_country(WIFI_COUNTRY_EU));
  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL));
  ESP_ERROR_CHECK(esp_wifi_start());

  esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);


  // I/O
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // display
#ifdef USE_DISPLAY
  display.init();
#ifdef FLIP_DISPLAY
  display.flipScreenVertically();
#endif

  /* show start screen */
  display.clear();
  display.setFont(ArialMT_Plain_16);
  display.drawString(6, 6, "PacketMonitor32");
  display.setFont(ArialMT_Plain_10);
  display.drawString(24, 34, "Made with <3 by");
  display.drawString(29, 44, "@Spacehuhn");
  display.display();

  delay(1000);
#endif

  // second core
  xTaskCreatePinnedToCore(
    coreTask,               /* Function to implement the task */
    "coreTask",             /* Name of the task */
    2500,                   /* Stack size in words */
    NULL,                   /* Task input parameter */
    0,                      /* Priority of the task */
    NULL,                   /* Task handle. */
    RUNNING_CORE);          /* Core where the task should run */

  // start Wifi sniffer
  esp_wifi_set_promiscuous_rx_cb(&wifi_promiscuous);
  esp_wifi_set_promiscuous(true);
}

void loop() {
  vTaskDelay(portMAX_DELAY);
}

void coreTask( void * p ) {

  uint32_t currentTime;

  while (true) {

    currentTime = millis();

    /* bit of spaghetti code, have to clean this up later :D */

    // check button
    if (digitalRead(BUTTON_PIN) == LOW) {
      if (buttonEnabled) {
        if (!buttonPressed) {
          buttonPressed = true;
          lastButtonTime = currentTime;
        } else if (currentTime - lastButtonTime >= 2000) {
          buttonPressed = false;
          buttonEnabled = false;
        }
      }
    } else {
      if (buttonPressed) {
        setChannel(ch + 1);
        draw();
      }
      buttonPressed = false;
      buttonEnabled = true;
    }


    // draw Display
    if ( currentTime - lastDrawTime > 500 ) {
      lastDrawTime = currentTime;
      // Serial.printf("\nFree RAM %u %u\n", heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT), heap_caps_get_minimum_free_size(MALLOC_CAP_32BIT));// for debug purposes

      pkts[MAX_X - 1] = tmpPacketCounter;

      draw();

      Serial.println((String)pkts[MAX_X - 1]);

      tmpPacketCounter = 0;
      deauths = 0;
      rssiSum = 0;
    }

    // Serial input
    if (Serial.available()) {
      ch = Serial.readString().toInt();
      if (ch < 1 || ch > 14) ch = 1;
      setChannel(ch);
    }

  }

}
