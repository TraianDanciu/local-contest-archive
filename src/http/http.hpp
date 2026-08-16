#pragma once

#include <string>

std::string http_get(std::string url);
void http_get_file(std::string url, std::string path);