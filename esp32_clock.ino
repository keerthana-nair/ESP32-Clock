// configure your connection here
#define USE_EDUROAM 1
#define WIFI_USER "REDACTED"
#define WIFI_SSID "eduroam"
#define WIFI_PASS "REDACTED"
// end of configuration

#include "WiFi.h"
#include "time.h"

const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec= 0;

#if USE_EDUROAM
#include "esp_eap_client.h"
const char* ssid = "eduroam";
const char* user = WIFI_USER;
#else
const char* ssid = WIFI_SSID;
#endif

const char* pass = WIFI_PASS;

// dump some details of the WiFi connection (if any) to the serial line
void print_wifi_info ()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.print("Connected to WiFi network: ");
    Serial.println(WiFi.SSID());
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Gateway address: ");
    Serial.println(WiFi.gatewayIP());
    Serial.print("Subnet mask: ");
    Serial.println(WiFi.subnetMask());
    Serial.print("MAC address: ");
    Serial.println(WiFi.macAddress());
  }
  else
  {
    Serial.println("WiFi not connected");
    Serial.print("MAC address: ");
    Serial.println(WiFi.macAddress());
  }
}

void wifi_connect ( float timeout = 15 )
{
  unsigned long deadline;

  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);

#if USE_EDUROAM
  Serial.printf("Connecting to eduroam as user %s\n", user);
  esp_eap_client_set_identity((uint8_t *)user, strlen(user));
  esp_eap_client_set_username((uint8_t *)user, strlen(user));
  esp_eap_client_set_password((uint8_t *)pass, strlen(pass));
  esp_wifi_sta_enterprise_enable();
  WiFi.begin(ssid);
#else
  Serial.printf("Connecting to %s\n", ssid);
  WiFi.begin(ssid, pass);
#endif
  
  deadline = millis() + (unsigned long)(timeout * 1000);

  while ((WiFi.status() != WL_CONNECTED) && (millis() < deadline))
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  print_wifi_info();
}

#include <SPI.h>
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

#define TFT_GREY 0xBDF7

float sx = 0, sy = 1, mx = 1, my = 0, hx = -1, hy = 0;    // Saved H, M, S x & y multipliers
float sdeg=0, mdeg=0, hdeg=0;
uint16_t osx=64, osy=64, omx=64, omy=64, ohx=64, ohy=64;  // Saved H, M, S x & y coords
uint16_t x0=0, x1=0, yy0=0, yy1=0;
uint32_t targetTime = 0;                    // for next 1 second timeout

static uint8_t conv2d(const char* p) {
  uint8_t v = 0;
  if ('0' <= *p && *p <= '9')
    v = *p - '0';
  return 10 * v + *++p - '0';
}

//uint8_t hh=conv2d(__TIME__), mm=conv2d(__TIME__+3), ss=conv2d(__TIME__+6);  // Get H, M, S from compile time
uint8_t hh=0, mm=0, ss=0;   // will be set from NTP

bool initial = 1;

void printLocalTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

bool updateTimeFromNTP() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return false;               // NTP not ready yet
  }
  hh = timeinfo.tm_hour;
  mm = timeinfo.tm_min;
  ss = timeinfo.tm_sec;
  return true;
}

void setup(void) {
  // put your setup code here, to run once:
  
  //WiFi related
  Serial.begin(115200);
  wifi_connect ();

  //Setting up screen display
  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_GREY);
  tft.setTextColor(TFT_RED, TFT_GREY);  // Adding a black background colour erases previous text automatically
  
  // Draw clock face
  tft.fillCircle(64, 64, 61, TFT_BLUE);
  tft.fillCircle(64, 64, 57, TFT_BLACK);

  // Draw 12 lines
  for(int i = 0; i<360; i+= 30) {
    sx = cos((i-90)*0.0174532925);
    sy = sin((i-90)*0.0174532925);
    x0 = sx*57+64;
    yy0 = sy*57+64;
    x1 = sx*50+64;
    yy1 = sy*50+64;

    tft.drawLine(x0, yy0, x1, yy1, TFT_BLUE);
  }

  // Draw 60 dots
  for(int i = 0; i<360; i+= 6) {
    sx = cos((i-90)*0.0174532925);
    sy = sin((i-90)*0.0174532925);
    x0 = sx*53+64;
    yy0 = sy*53+64;
    
    tft.drawPixel(x0, yy0, TFT_BLUE);
    if(i==0 || i==180) tft.fillCircle(x0, yy0, 1, TFT_CYAN);
    if(i==0 || i==180) tft.fillCircle(x0+1, yy0, 1, TFT_CYAN);
    if(i==90 || i==270) tft.fillCircle(x0, yy0, 1, TFT_CYAN);
    if(i==90 || i==270) tft.fillCircle(x0+1, yy0, 1, TFT_CYAN);
  }

  tft.fillCircle(65, 65, 3, TFT_RED);

  targetTime = millis() + 1000; 
   // Configure NTP time synchronization
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  
  // Wait a moment and print time
  delay(2000);
  printLocalTime();

}


void loop() {
  if (targetTime < millis()) {
    targetTime = millis() + 1000;
    printLocalTime();

    if (!updateTimeFromNTP()) return;   // skip drawing until we have real time

    // Pre-compute hand degrees, x & y coords
    sdeg = ss*6;
    mdeg = mm*6 + sdeg*0.01666667;
    hdeg = hh*30 + mdeg*0.0833333;
    hx = cos((hdeg-90)*0.0174532925);
    hy = sin((hdeg-90)*0.0174532925);
    mx = cos((mdeg-90)*0.0174532925);
    my = sin((mdeg-90)*0.0174532925);
    sx = cos((sdeg-90)*0.0174532925);
    sy = sin((sdeg-90)*0.0174532925);

    if (ss==0 || initial) {
      initial = 0;
      // Erase hour and minute hands
      tft.drawLine(ohx, ohy, 65, 65, TFT_BLACK);
      ohx = hx*33+65;
      ohy = hy*33+65;
      tft.drawLine(omx, omy, 65, 65, TFT_BLACK);
      omx = mx*44+65;
      omy = my*44+65;
    }

    // Redraw hands
    tft.drawLine(osx, osy, 65, 65, TFT_BLACK);
    tft.drawLine(ohx, ohy, 65, 65, TFT_WHITE);
    tft.drawLine(omx, omy, 65, 65, TFT_WHITE);
    osx = sx*47+65;
    osy = sy*47+65;
    tft.drawLine(osx, osy, 65, 65, TFT_RED);

    tft.fillCircle(65, 65, 3, TFT_RED);
  }
}

