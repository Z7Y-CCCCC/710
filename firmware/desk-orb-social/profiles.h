#pragma once

struct SocialProfileSource {
  const char* id;
  const char* name;
  const char* url;
  const char* statsUrl;
  const char* fallbackNickname;
  const char* avatarText;
  uint16_t color565;
};

SocialProfileSource PROFILE_SOURCES[] = {
  {
    "bilibili",
    "哔哩哔哩",
    "https://space.bilibili.com/3546867614878349",
    "https://api.bilibili.com/x/relation/stat?vmid=3546867614878349",
    "B站账号",
    "B",
    0x04DF,
  },
  {
    "douyin",
    "抖音",
    "https://www.douyin.com/user/MS4wLjABAAAAaUGAN5swqwZMFuriuXXeScUcZzH1j5ndwD4Gzv4Zd9Bv7LoAFHmQIS0IAX7Ouodp?from_tab_name=main",
    "",
    "抖音号 63230246827",
    "抖",
    0x0841,
  },
  {
    "toutiao",
    "今日头条",
    "https://www.toutiao.com/c/user/token/CizUGDjTPuJYRMOhlDkdXL8xsa39ne_PlyoibC8UUJuXqTydoGl2_SI-rYQnwBpJCjwAAAAAAAAAAAAAUKQRThn1KrIcHHVtQ8jtH1FFQLtb95i6b--tMSTCc6buo8LknRThC8MD6wNKYzgSnN8Q47WWDhjDxYPqBCIBA0AOo8M=/?tab=weitoutiao",
    "",
    "仙翁777",
    "头",
    0xD8E4,
  },
};

const int PROFILE_COUNT = sizeof(PROFILE_SOURCES) / sizeof(PROFILE_SOURCES[0]);
