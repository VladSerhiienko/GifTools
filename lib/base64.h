#pragma once
#include <vector>

std::string base64_encode_string(const uint8_t* buffer, size_t bufferLength);
std::vector<uint8_t> base64_encode(const uint8_t* buffer, size_t bufferLength);
std::vector<uint8_t> base64_decode(const uint8_t* base64, size_t base64Length);
