#include "base64.h"

#include <string>

namespace {
constexpr std::string_view base64_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

bool is_base64(uint8_t c) { return (isalnum(c) || (c == '+') || (c == '/')); }
} // namespace

std::vector<uint8_t> base64_encode(const uint8_t* bytes_to_encode, size_t in_len) {
    std::vector<uint8_t> ret;

    // https://stackoverflow.com/questions/13378815/base64-length-calculation
    ret.reserve(4 * (in_len / 3) + 1);

    int32_t i = 0;
    int32_t j = 0;
    uint8_t char_array_3[3];
    uint8_t char_array_4[4];

    while (in_len--) {
        char_array_3[i++] = *(bytes_to_encode++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (i = 0; (i < 4); i++) ret.push_back(base64_chars[char_array_4[i]]);
            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 3; j++) char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);

        for (j = 0; (j < i + 1); j++) ret.push_back(base64_chars[char_array_4[j]]);

        while ((i++ < 3)) ret.push_back('=');
    }

    ret.push_back(0);
    return ret;
}

std::vector<uint8_t> base64_decode(const uint8_t* encoded_string, size_t in_len) {
    int32_t i = 0;
    int32_t j = 0;
    int32_t in_ = 0;
    uint8_t char_array_4[4] = {0}, char_array_3[3] = {0};

    std::vector<uint8_t> ret;
    // https://stackoverflow.com/questions/13378815/base64-length-calculation
    ret.reserve(in_len * 3 / 4);

    while (in_len-- && (encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
        char_array_4[i++] = encoded_string[in_];
        in_++;
        if (i == 4) {
            for (i = 0; i < 4; i++) char_array_4[i] = base64_chars.find(char_array_4[i]) & 0xff;

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (i = 0; (i < 3); i++) { ret.push_back(char_array_3[i]); }
            i = 0;
        }
    }

    if (i) {
        for (j = 0; j < i; j++) char_array_4[j] = base64_chars.find(char_array_4[j]) & 0xff;

        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);

        for (j = 0; (j < i - 1); j++) { ret.push_back(char_array_3[j]); }
    }

    return ret;
}
