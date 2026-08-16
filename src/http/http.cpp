#include "http.hpp"
#include <curl/curl.h>
#include <cstdio>
#include <stdexcept>

static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
  ((std::string*)userp)->append((char*)contents, size * nmemb);
  return size * nmemb;
}

static size_t write_file_callback(void *contents, size_t size, size_t nmemb, void *userp) {
  return fwrite(contents, size, nmemb, (FILE*)userp);
}

std::string http_get(std::string url) {
  CURL *curl = curl_easy_init();

  if(curl == nullptr) {
    throw std::runtime_error("Failed to initialize CURL.");
  }

  std::string response;
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (X11; Linux x86_64; rv:141.0) Gecko/20100101 Firefox/141.0");
  curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
  curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2TLS);
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15L);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);

  CURLcode res = curl_easy_perform(curl);

  long http_code = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
  curl_easy_cleanup(curl);

  if(res != CURLE_OK) {
    throw std::runtime_error(curl_easy_strerror(res));
  }
  if(http_code != 200) {
    throw std::runtime_error("HTTP " + std::to_string(http_code) + " for " + url);
  }

  return response;
}

void http_get_file(std::string url, std::string path) {
  CURL *curl = curl_easy_init();

  if(curl == nullptr) {
    throw std::runtime_error("Failed to initialize CURL.");
  }

  FILE *file = std::fopen(path.c_str(), "wb");
  if(file == nullptr) {
    curl_easy_cleanup(curl);
    throw std::runtime_error("Failed to open file for writing: " + path);
  }

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_file_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (X11; Linux x86_64; rv:141.0) Gecko/20100101 Firefox/141.0");
  curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
  curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2TLS);
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15L);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 600L);

  CURLcode res = curl_easy_perform(curl);

  long http_code = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
  curl_easy_cleanup(curl);
  std::fclose(file);

  if(res != CURLE_OK) {
    throw std::runtime_error(curl_easy_strerror(res));
  }
  if(http_code != 200) {
    throw std::runtime_error("HTTP " + std::to_string(http_code) + " for " + url);
  }
}
