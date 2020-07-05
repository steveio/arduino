/**
 * ESP32 BMP280 Sensor, 0.96 OLED LCD 
 * 
 * Timer wake from deep sleep, read sensor, transmit data over wifi to MQTT server
 * 
 * Wifi dynamic parameters: IP, Subnet Mask, Gateway, DNS are cached during sleep & restored on wake
 * 
 */

#include <Adafruit_BMP280.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <PubSubClient.h>


/*
I2C device found at address 0x3C
I2C device found at address 0x76
*/

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET    -1 // Reset pin # (or -1 if sharing Arduino reset pin)

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); //Declaring the display name (display)

Adafruit_BMP280 bmp;


const char* ssid = "__SSID__";
const char* pass = "__PASS__";

const char* mqtt_server = "192.168.1.127";
const char* mqtt_channel_pub = "esp32.out";
const char* mqtt_channel_sub = "esp32.in";


WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (128)
char msg[MSG_BUFFER_SIZE];
int value = 0;


#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  15         /* Sleep Time (in seconds) */


static RTC_DATA_ATTR struct {
    byte mac [ 6 ];
    byte mode;
    byte chl;
    uint32_t ip;
    uint32_t gw;
    uint32_t msk;
    uint32_t dns;
    uint32_t seq;
    uint32_t chk;
} cfgbuf;

uint32_t startMs, wifiConnMs, MqttConnMs;

bool checkCfg() {
    uint32_t x = 0;
    uint32_t *p = (uint32_t *)cfgbuf.mac;
    for (uint32_t i = 0; i < sizeof(cfgbuf)/4; i++) x += p[i];
    //printf("RTC read: chk=%x x=%x ip=%08x mode=%d %s\n", cfgbuf.chk, x, cfgbuf.ip, cfgbuf.mode,
    //        x==0?"OK":"FAIL");
    if (x == 0 && cfgbuf.ip != 0) return true;
    printf("NVRAM cfg init\n");
    // bad checksum, init data
    for (uint32_t i = 0; i < 6; i++) cfgbuf.mac[i] = 0xff;
    cfgbuf.mode = 0; // chk err, reconfig
    cfgbuf.chl = 0;
    cfgbuf.ip = IPAddress(0, 0, 0, 0);
    cfgbuf.gw = IPAddress(0, 0, 0, 0);
    cfgbuf.msk = IPAddress(255, 255, 255, 0);
    cfgbuf.dns = IPAddress(0, 0, 0, 0);
    cfgbuf.seq = 100;
    return false;
}

void writecfg(void) {
    // save new info
    uint8_t *bssid = WiFi.BSSID();
    for (int i=0; i<sizeof(cfgbuf.mac); i++) cfgbuf.mac[i] = bssid[i];
    cfgbuf.chl = WiFi.channel();
    cfgbuf.ip = WiFi.localIP();
    cfgbuf.gw = WiFi.gatewayIP();
    cfgbuf.msk = WiFi.subnetMask();
    cfgbuf.dns = WiFi.dnsIP();
    // recalculate checksum
    uint32_t x = 0;
    uint32_t *p = (uint32_t *)cfgbuf.mac;
    for (uint32_t i = 0; i < sizeof(cfgbuf)/4-1; i++) x += p[i];
    cfgbuf.chk = -x;
    //printf("RTC write: chk=%x x=%x ip=%08x mode=%d\n", cfgbuf.chk, x, cfgbuf.ip, cfgbuf.mode);
}


void setup_wifi() {


    // Read config from NVRAM
#ifdef FORCE_MODE
    if (checkCfg()) cfgbuf.mode = FORCE_MODE; // can only force if we got saved info
#else
    checkCfg();
#endif

    // Make sure Wifi settings in flash are off so it doesn't start automagically at next boot
    if (WiFi.getMode() != WIFI_OFF) {
        printf("Wifi wasn't off!\n");
        WiFi.persistent(true);
        WiFi.mode(WIFI_OFF);
    }

    // Init Wifi in STA mode and connect
    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);
    int m = cfgbuf.mode;
    bool ok;
    switch (cfgbuf.mode) {
    case 0:
        ok = WiFi.begin(ssid, pass);
        break;
    case 1:
        ok = WiFi.begin(ssid, pass, cfgbuf.chl, cfgbuf.mac);
        break;
    case 2:
        ok = WiFi.config(cfgbuf.ip, cfgbuf.gw, cfgbuf.msk, cfgbuf.dns);
        if (!ok) printf("*** Wifi.config failed, mode=%d\n", m);
        ok = WiFi.begin(ssid, pass);
        break;
    default:
        ok = WiFi.config(cfgbuf.ip, cfgbuf.gw, cfgbuf.msk, cfgbuf.dns);
        if (!ok) printf("*** Wifi.config failed, mode=%d\n", m);
        ok = WiFi.begin(ssid, pass, cfgbuf.chl, cfgbuf.mac);
        //printf("BSSID: %x:%x:%x:%x:%x:%x\n", cfgbuf.mac[0], cfgbuf.mac[1], cfgbuf.mac[2],
        //    cfgbuf.mac[3], cfgbuf.mac[4], cfgbuf.mac[5]);
        cfgbuf.mode = -1;
        break;
    }
    if (!ok) {
        printf("*** Wifi.begin failed, mode=%d\n", m);
        deep_sleep();
    }
    while (WiFi.status() != WL_CONNECTED) delay(1);
    uint32_t wifiConnMs = millis();
    printf("Wifi connect in %dms\n", wifiConnMs);

}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
  } else {
  }

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(mqtt_channel_pub, "hello ESP32");
      // ... and resubscribe
      client.subscribe(mqtt_channel_sub);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}


void deep_sleep() {

  printf("Sleep at %d ms\n\n", millis());
  delay(20);

  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) + " Seconds");

  //esp_deep_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
  //Serial.println("Configured all RTC Peripherals to be powered down in sleep");

  Serial.println("Going to sleep now");
  Serial.flush(); 
  esp_deep_sleep_start();
}


void setup() {
  
  Serial.begin(115200);
  
  print_wakeup_reason();
  
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
    
  uint32_t mqttConnMs = millis();
  printf("Mode %d, Init %d ms, Wifi %d ms, Mqtt %d ms, seq=%d, SSID %s, IDF %s\n",
          cfgbuf.mode, startMs, wifiConnMs, mqttConnMs, cfgbuf.seq,ssid, esp_get_idf_version());

  cfgbuf.mode++;
  writecfg();


  Serial.println(F("BMP280, OLED init"));
  
  if (!bmp.begin()) {
    Serial.println(F("Could not find a valid BMP280 sensor, check wiring!"));
    while (1);
  }
  
  /* Default settings from datasheet. */
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */
  
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); //Start the OLED display
  display.clearDisplay();
  display.display();
  delay(2000);

}

void loop() {

  print_wakeup_reason();

  Serial.print(F("Temperature = "));
  Serial.print(bmp.readTemperature());
  Serial.println(" *C");

  Serial.print(F("Pressure = "));
  Serial.print(bmp.readPressure()/100); //displaying the Pressure in hPa, you can change the unit
  Serial.println(" hPa");

  Serial.print(F("Approx altitude = "));
  Serial.print(bmp.readAltitude(1019.66)); //The "1019.66" is the pressure(hPa) at sea level in day in your region
  Serial.println(" m");                    //If you don't know it, modify it until you get your current altitude

  display.clearDisplay();
  float T = bmp.readTemperature();           //Read temperature in C
  float P = bmp.readPressure()/100;         //Read Pressure in Pa and conversion to hPa
  float A = bmp.readAltitude(1019.66);      //Calculating the Altitude, the "1019.66" is the pressure in (hPa) at sea level at day in your region
                                            //If you don't know it just modify it until you get the altitude of your place
  
  display.setCursor(0,0);
  display.setTextColor(WHITE);
  display.setTextSize(2); 
  display.print("Temp");
  
  display.setCursor(0,18);
  display.print(T,1);
  display.setCursor(50,17);
  display.setTextSize(1);
  display.print("C");

  display.setTextSize(1);
  display.setCursor(65,0);
  display.print("Pres");
  display.setCursor(65,10);
  display.print(P);
  display.setCursor(110,10);
  display.print("hPa");

  display.setCursor(65,25);
  display.print("Alt");
  display.setCursor(90,25);
  display.print(A,0);
  display.setCursor(110,25);
  display.print("m");
  
  display.display();

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  delay(500);

  snprintf(msg, MSG_BUFFER_SIZE, "%.2f,%.2f,%.2f", T, P, A);

  Serial.print("Publish message: ");
  Serial.println(msg);
  client.publish(mqtt_channel_pub, msg);

  deep_sleep();
}
