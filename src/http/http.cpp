#include "http.hpp"

#include <curl/curl.h>

#include <stdexcept>

static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
  ((std::string*)userp)->append((char*)contents, size * nmemb);
  return size * nmemb;
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

  CURLcode res = curl_easy_perform(curl);
  curl_easy_cleanup(curl);

  if(res != CURLE_OK) {
    throw std::runtime_error(curl_easy_strerror(res));
  }

  return response;
}