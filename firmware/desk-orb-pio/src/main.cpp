#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <Arduino_GFX_Library.h>

#define TFT_MOSI 47
#define TFT_SCLK 21
#define TFT_CS   41
#define TFT_DC   40
#define TFT_RST  45
#define TFT_BL   42
#define BOOT_BTN 0

const char* DAILY_PICK_URL = "https://z7y-ccccc.github.io/710/today.json";
const char* FALLBACK_COVER_RGB565_URL = "https://z7y-ccccc.github.io/710/cover-72x96.rgb565";
const char* BILI_STAT_URL = "https://api.bilibili.com/x/relation/stat?vmid=3546867614878349";
const int COVER_W = 72;
const int COVER_H = 96;
const size_t COVER_BYTES = COVER_W * COVER_H * 2;

Arduino_DataBus *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCLK, TFT_MOSI, GFX_NOT_DEFINED, HSPI, true);
Arduino_GFX *gfx = new Arduino_ST7789(bus, TFT_RST, 0, true, 240, 240, 0, 0);

Preferences prefs;
WebServer server(80);

String ssid;
String pass;
String pickCode = "--";
String pickTitle = "loading...";
String pickTags = "";
String pickCover = "";
String pickCoverRgb565 = "";
String pickMaker = "";
String biliFans = "--";
String statusLine = "starting";
String coverStatus = "cover not loaded";
unsigned long lastFetch = 0;
int page = 0;
unsigned long lastPage = 0;
bool coverReady = false;
uint16_t coverPixels[COVER_W * COVER_H];

uint16_t C_BG = RGB565(238, 244, 248);
uint16_t C_INK = RGB565(28, 38, 51);
uint16_t C_BLUE = RGB565(79, 142, 247);
uint16_t C_GREEN = RGB565(73, 184, 131);
uint16_t C_RED = RGB565(215, 25, 32);
uint16_t C_PANEL = RGB565(255, 255, 255);

String htmlEscape(String s) {
  s.replace("&", "&amp;");
  s.replace("<", "&lt;");
  s.replace(">", "&gt;");
  s.replace("\"", "&quot;");
  return s;
}

String shortNum(long v) {
  if (v <= 0) return "--";
  if (v >= 10000) return String((float)v / 10000.0, v >= 100000 ? 0 : 1) + "w";
  if (v >= 1000) return String((float)v / 1000.0, 1) + "k";
  return String(v);
}

void drawHeader(const char* mode) {
  gfx->fillRoundRect(12, 10, 216, 32, 7, C_PANEL);
  gfx->setTextColor(C_INK);
  gfx->setTextSize(1);
  gfx->setCursor(22, 22);
  gfx->print("DESK ORB");
  gfx->setCursor(178, 22);
  gfx->print(mode);
}

void drawFooter(const String& text) {
  gfx->fillRoundRect(12, 202, 216, 28, 7, C_PANEL);
  gfx->setTextColor(C_INK);
  gfx->setTextSize(1);
  gfx->setCursor(22, 212);
  String s = text;
  if (s.length() > 30) s = s.substring(0, 30);
  gfx->print(s);
}

void drawLogoCircle(int x, int y, uint16_t color, const char* txt) {
  gfx->fillCircle(x, y, 29, color);
  gfx->setTextColor(WHITE);
  gfx->setTextSize(2);
  gfx->setCursor(x - 12, y - 8);
  gfx->print(txt);
}

void drawAccountPage() {
  gfx->fillScreen(C_BG);
  drawHeader("RADAR");
  gfx->fillRoundRect(18, 58, 204, 126, 8, C_PANEL);
  drawLogoCircle(58, 95, RGB565(0, 161, 214), "B");
  gfx->setTextColor(C_INK);
  gfx->setTextSize(2);
  gfx->setCursor(96, 75);
  gfx->print("Bilibili");
  gfx->setTextSize(1);
  gfx->setCursor(96, 101);
  gfx->print("UID 3546867614878349");
  gfx->fillRoundRect(30, 134, 180, 36, 7, RGB565(235, 245, 255));
  gfx->setTextColor(C_BLUE);
  gfx->setTextSize(3);
  gfx->setCursor(44, 142);
  gfx->print(biliFans);
  gfx->setTextColor(C_INK);
  gfx->setTextSize(1);
  gfx->setCursor(145, 151);
  gfx->print("fans");
  drawFooter(statusLine);
}

void drawCoverSlot() {
  gfx->fillRoundRect(28, 68, COVER_W, COVER_H, 7, RGB565(30, 36, 48));
  if (coverReady) {
    gfx->draw16bitRGBBitmap(28, 68, coverPixels, COVER_W, COVER_H);
    gfx->drawRoundRect(28, 68, COVER_W, COVER_H, 7, RGB565(30, 36, 48));
    return;
  }
  gfx->setTextColor(RGB565(242, 184, 75));
  gfx->setTextSize(2);
  gfx->setCursor(38, 94);
  gfx->print("TODAY");
  gfx->setTextSize(1);
  gfx->setCursor(42, 122);
  gfx->print("COVER");
}

void drawPickPage() {
  gfx->fillScreen(C_BG);
  drawHeader("PICK");
  gfx->fillRoundRect(16, 54, 208, 142, 9, C_PANEL);
  drawCoverSlot();

  gfx->setTextColor(C_RED);
  gfx->setTextSize(2);
  gfx->setCursor(112, 70);
  gfx->print(pickCode.substring(0, 10));

  gfx->setTextColor(C_INK);
  gfx->setTextSize(1);
  String title = pickTitle;
  if (title.length() > 56) title = title.substring(0, 56) + "...";
  int y = 100;
  for (int i = 0; i < title.length(); i += 18) {
    gfx->setCursor(112, y);
    gfx->print(title.substring(i, min(i + 18, (int)title.length())));
    y += 13;
    if (y > 150) break;
  }
  gfx->setCursor(112, 166);
  gfx->setTextColor(C_GREEN);
  gfx->print(pickTags.substring(0, 20));
  if (coverReady) {
    drawFooter(pickMaker.length() ? pickMaker : "cover ok");
  } else {
    drawFooter(coverStatus);
  }
}

void drawSetupPage(const String& ipText) {
  gfx->fillScreen(C_BG);
  drawHeader("SETUP");
  gfx->setTextColor(C_INK);
  gfx->setTextSize(2);
  gfx->setCursor(30, 72);
  gfx->print("WiFi Setup");
  gfx->setTextSize(1);
  gfx->setCursor(30, 112);
  gfx->print("Connect AP:");
  gfx->setCursor(30, 130);
  gfx->print("DeskOrb-Setup");
  gfx->setCursor(30, 154);
  gfx->print("Open:");
  gfx->setCursor(30, 172);
  gfx->print(ipText);
  drawFooter("save WiFi then reboot");
}

void handleRoot() {
  String pageHtml = "<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'><meta charset='utf-8'><title>DeskOrb WiFi</title></head><body style='font-family:sans-serif;padding:24px'><h2>DeskOrb WiFi</h2><form method='post' action='/save'>SSID<br><input name='s' style='width:100%;height:36px' value='" + htmlEscape(ssid) + "'><br><br>Password<br><input name='p' type='password' style='width:100%;height:36px'><br><br><button style='height:40px'>Save & Reboot</button></form></body></html>";
  server.send(200, "text/html; charset=utf-8", pageHtml);
}

void handleSave() {
  ssid = server.arg("s");
  pass = server.arg("p");
  prefs.begin("wifi", false);
  prefs.putString("ssid", ssid);
  prefs.putString("pass", pass);
  prefs.end();
  server.send(200, "text/html; charset=utf-8", "Saved. Rebooting...");
  delay(800);
  ESP.restart();
}

bool connectWiFi() {
  prefs.begin("wifi", true);
  ssid = prefs.getString("ssid", "");
  pass = prefs.getString("pass", "");
  prefs.end();
  if (ssid.length() == 0) return false;
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());
  statusLine = "connecting " + ssid;
  drawAccountPage();
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) delay(250);
  return WiFi.status() == WL_CONNECTED;
}

void startPortal() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP("DeskOrb-Setup");
  IPAddress ip = WiFi.softAPIP();
  server.on("/", HTTP_GET, handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.begin();
  drawSetupPage("http://" + ip.toString());
}

String httpsGet(const char* url) {
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.setUserAgent("DeskOrb ESP32S3");
  if (!http.begin(client, url)) return "";
  int code = http.GET();
  String body = code >= 200 && code < 300 ? http.getString() : "";
  http.end();
  return body;
}

bool httpsGetBytes(const String& url, uint8_t* target, size_t expected, String& reason) {
  if (!url.length()) {
    reason = "cover no url";
    return false;
  }
  WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(20000);
  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.setUserAgent("Mozilla/5.0 DeskOrb ESP32S3");
  http.setTimeout(20000);
  if (!http.begin(client, url)) {
    reason = "cover begin fail";
    return false;
  }
  int code = http.GET();
  if (code < 200 || code >= 300) {
    reason = "cover http " + String(code);
    http.end();
    return false;
  }
  WiFiClient* stream = http.getStreamPtr();
  size_t got = 0;
  unsigned long deadline = millis() + 25000;
  while (got < expected && millis() < deadline) {
    int available = stream->available();
    if (available > 0) {
      size_t want = min((size_t)available, expected - got);
      int readNow = stream->readBytes(target + got, want);
      if (readNow > 0) {
        got += (size_t)readNow;
        deadline = millis() + 8000;
      }
    } else {
      delay(10);
    }
  }
  http.end();
  if (got != expected) {
    reason = "cover bytes " + String(got) + "/" + String(expected);
    return false;
  }
  reason = "cover ok";
  return true;
}

void fetchBili() {
  String body = httpsGet(BILI_STAT_URL);
  JsonDocument doc;
  if (deserializeJson(doc, body) == DeserializationError::Ok) {
    long fans = doc["data"]["follower"] | 0;
    biliFans = shortNum(fans);
  }
}

void fetchCover() {
  coverReady = false;
  String url = pickCoverRgb565.length() ? pickCoverRgb565 : String(FALLBACK_COVER_RGB565_URL);
  bool ok = httpsGetBytes(url, (uint8_t*)coverPixels, COVER_BYTES, coverStatus);
  if (!ok && url != FALLBACK_COVER_RGB565_URL) {
    ok = httpsGetBytes(String(FALLBACK_COVER_RGB565_URL), (uint8_t*)coverPixels, COVER_BYTES, coverStatus);
  }
  coverReady = ok;
}

void fetchPick() {
  String body = httpsGet(DAILY_PICK_URL);
  JsonDocument doc;
  if (deserializeJson(doc, body) != DeserializationError::Ok) {
    pickCode = "PICK";
    pickTitle = "daily JSON loading";
    pickCoverRgb565 = FALLBACK_COVER_RGB565_URL;
    fetchCover();
    return;
  }
  if (!(doc["ok"] | false)) {
    pickCode = "NO DATA";
    pickTitle = doc["error"] | "today.json not ready";
    pickCoverRgb565 = FALLBACK_COVER_RGB565_URL;
    fetchCover();
    return;
  }
  pickCode = doc["pick"]["code"] | "--";
  pickTitle = doc["pick"]["title"] | "--";
  pickMaker = doc["pick"]["maker"] | "";
  pickCover = doc["pick"]["cover"] | "";
  pickCoverRgb565 = doc["pick"]["coverRgb565"] | "";
  if (!pickCoverRgb565.length()) {
    pickCoverRgb565 = FALLBACK_COVER_RGB565_URL;
  }
  pickTags = "";
  JsonArray tags = doc["pick"]["tags"].as<JsonArray>();
  for (JsonVariant t : tags) {
    if (pickTags.length()) pickTags += ", ";
    pickTags += t.as<String>();
    if (pickTags.length() > 36) break;
  }
  fetchCover();
}

void refreshData() {
  if (WiFi.status() != WL_CONNECTED) return;
  statusLine = WiFi.localIP().toString();
  fetchBili();
  fetchPick();
  lastFetch = millis();
}

void setup() {
  Serial.begin(115200);
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  pinMode(BOOT_BTN, INPUT_PULLUP);
  gfx->begin(40000000);
  gfx->fillScreen(C_BG);
  statusLine = "booting";
  drawAccountPage();

  bool ok = connectWiFi();
  if (!ok) {
    startPortal();
    return;
  }
  refreshData();
  drawAccountPage();
}

void loop() {
  server.handleClient();
  if (WiFi.getMode() == WIFI_AP) return;
  if (millis() - lastFetch > 30UL * 60UL * 1000UL) refreshData();
  if (millis() - lastPage > 5500) {
    page = (page + 1) % 2;
    lastPage = millis();
    if (page == 0) drawAccountPage(); else drawPickPage();
  }
  if (digitalRead(BOOT_BTN) == LOW) {
    delay(30);
    if (digitalRead(BOOT_BTN) == LOW) {
      page = (page + 1) % 2;
      if (page == 0) drawAccountPage(); else drawPickPage();
      while (digitalRead(BOOT_BTN) == LOW) delay(10);
    }
  }
}
