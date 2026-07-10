#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <TFT_eSPI.h>
#include "profiles.h"

const char* WIFI_SSID = "填你的WiFi名称";
const char* WIFI_PASSWORD = "填你的WiFi密码";

TFT_eSPI tft = TFT_eSPI();

struct SocialProfile {
  String id;
  String name;
  String nickname;
  String avatarText;
  String avatarUrl;
  uint32_t followers;
  uint16_t color565;
  bool ok;
};

SocialProfile profiles[PROFILE_COUNT];
int activeProfile = 0;
unsigned long lastFetchAt = 0;
unsigned long lastRotateAt = 0;

const unsigned long FETCH_INTERVAL_MS = 30UL * 60UL * 1000UL;
const unsigned long ROTATE_INTERVAL_MS = 5200UL;

String extractBetween(const String& source, const String& left, const String& right) {
  int start = source.indexOf(left);
  if (start < 0) return "";
  start += left.length();
  int end = source.indexOf(right, start);
  if (end < 0) return "";
  return source.substring(start, end);
}

String extractFirstMatch(const String& html, const char* keys[], int count) {
  for (int i = 0; i < count; i += 1) {
    String key = keys[i];
    String value = extractBetween(html, key + "\":\"", "\"");
    if (value.length() > 0) return value;
    value = extractBetween(html, key + "\\\":\\\"", "\\\"");
    if (value.length() > 0) return value;
  }
  return "";
}

uint32_t extractFollowers(const String& html) {
  const char* keys[] = {
    "follower",
    "follower_count",
    "followers_count",
    "fans_count",
    "fans",
    "粉丝"
  };

  for (int i = 0; i < 5; i += 1) {
    String key = keys[i];
    int pos = html.indexOf(key);
    if (pos < 0) continue;
    int cursor = pos + key.length();
    while (cursor < html.length() && !isDigit(html[cursor])) cursor += 1;
    String digits = "";
    while (cursor < html.length() && (isDigit(html[cursor]) || html[cursor] == '.')) {
      digits += html[cursor];
      cursor += 1;
    }
    if (digits.length() > 0) return digits.toFloat();
  }
  return 0;
}

bool fetchUrl(const char* url, String& body) {
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.setUserAgent("Mozilla/5.0 (ESP32 DeskOrb; public-profile-display)");

  if (!http.begin(client, url)) return false;
  int code = http.GET();
  if (code < 200 || code >= 400) {
    http.end();
    return false;
  }

  body = http.getString();
  http.end();
  return true;
}

bool fetchProfile(int index) {
  String html = "";
  bool pageFetched = fetchUrl(PROFILE_SOURCES[index].url, html);

  const char* nicknameKeys[] = {"nickname", "user_name", "screen_name", "name", "title"};
  const char* avatarKeys[] = {"avatar", "avatar_url", "avatarUrl", "profile_image_url"};

  String nickname = pageFetched ? extractFirstMatch(html, nicknameKeys, 5) : "";
  String avatarUrl = pageFetched ? extractFirstMatch(html, avatarKeys, 4) : "";
  uint32_t followers = pageFetched ? extractFollowers(html) : 0;

  if (strlen(PROFILE_SOURCES[index].statsUrl) > 0) {
    String stats = "";
    if (fetchUrl(PROFILE_SOURCES[index].statsUrl, stats)) {
      uint32_t apiFollowers = extractFollowers(stats);
      if (apiFollowers > 0) followers = apiFollowers;
    }
  }

  profiles[index].nickname = nickname.length() ? nickname : PROFILE_SOURCES[index].fallbackNickname;
  profiles[index].avatarUrl = avatarUrl;
  profiles[index].followers = followers > 0 ? followers : profiles[index].followers;
  profiles[index].ok = followers > 0 || nickname.length() > 0 || pageFetched;
  return profiles[index].ok;
}

String shortNumber(uint32_t value) {
  if (value >= 10000) {
    float w = value / 10000.0;
    return String(w, value >= 100000 ? 0 : 1) + "w";
  }
  if (value >= 1000) {
    return String(value / 1000.0, 1) + "k";
  }
  return String(value);
}

void drawRoundBox(int x, int y, int w, int h, int r, uint16_t color) {
  tft.fillRoundRect(x, y, w, h, r, color);
}

void drawLogo(const SocialProfile& profile, int x, int y, int radius) {
  tft.fillCircle(x, y, radius, profile.color565);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE, profile.color565);

  if (profile.id == "bilibili") {
    tft.fillRoundRect(x - 18, y - 11, 36, 24, 5, TFT_WHITE);
    tft.setTextColor(profile.color565, TFT_WHITE);
    tft.drawString("B", x, y + 1, 4);
  } else if (profile.id == "toutiao") {
    tft.drawString("头条", x, y, 2);
  } else if (profile.id == "xiaohongshu") {
    tft.drawString("小红", x, y, 2);
  } else {
    tft.drawString(profile.avatarText, x, y, 4);
  }
}

void drawProfileScreen() {
  const SocialProfile& profile = profiles[activeProfile];
  tft.fillScreen(TFT_BLACK);
  tft.fillRect(0, 0, 240, 240, TFT_WHITE);
  tft.fillRoundRect(12, 12, 216, 34, 7, 0xF7BE);

  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(0x2128, 0xF7BE);
  tft.drawString("DESK ORB", 22, 29, 2);
  tft.setTextDatum(MR_DATUM);
  tft.drawString("PUBLIC", 218, 29, 2);

  tft.fillRoundRect(18, 58, 204, 128, 8, 0xFFFF);
  drawLogo(profile, 58, 96, 26);

  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(0x2128, 0xFFFF);
  tft.drawString(profile.name, 96, 82, 2);
  tft.drawString(profile.nickname, 96, 103, 2);

  tft.fillRoundRect(28, 138, 184, 36, 7, 0xEF7D);
  tft.setTextColor(profile.color565, 0xEF7D);
  tft.drawString(shortNumber(profile.followers), 42, 156, 4);
  tft.setTextColor(0x5AEB, 0xEF7D);
  tft.drawString("粉丝", 142, 158, 2);

  tft.fillRoundRect(14, 198, 212, 28, 7, 0xF7BE);
  tft.setTextColor(0x2128, 0xF7BE);
  tft.setTextDatum(ML_DATUM);
  tft.drawString(profile.ok ? "公开资料已同步" : "显示缓存资料", 24, 212, 2);
}

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(350);
  }
}

void setupProfiles() {
  for (int i = 0; i < PROFILE_COUNT; i += 1) {
    profiles[i].id = PROFILE_SOURCES[i].id;
    profiles[i].name = PROFILE_SOURCES[i].name;
    profiles[i].nickname = PROFILE_SOURCES[i].fallbackNickname;
    profiles[i].avatarText = PROFILE_SOURCES[i].avatarText;
    profiles[i].followers = 0;
    profiles[i].color565 = PROFILE_SOURCES[i].color565;
    profiles[i].ok = false;
  }
}

void refreshAllProfiles() {
  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  for (int i = 0; i < PROFILE_COUNT; i += 1) {
    fetchProfile(i);
    delay(500);
  }
  lastFetchAt = millis();
}

void setup() {
  setupProfiles();
  tft.init();
  tft.setRotation(0);
  tft.setTextWrap(false);
  connectWiFi();
  refreshAllProfiles();
  drawProfileScreen();
}

void loop() {
  unsigned long now = millis();
  if (now - lastFetchAt > FETCH_INTERVAL_MS) refreshAllProfiles();
  if (now - lastRotateAt > ROTATE_INTERVAL_MS) {
    activeProfile = (activeProfile + 1) % PROFILE_COUNT;
    lastRotateAt = now;
    drawProfileScreen();
  }
}
