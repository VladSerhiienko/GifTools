//
//  base64 encoding and decoding with C++.
//  Version: 1.01.00
//

#ifndef BASE64_H_C0CE2A47_D10E_42C9_A27C_C883944E704A
#define BASE64_H_C0CE2A47_D10E_42C9_A27C_C883944E704A

#include <vector>

std::vector<uint8_t> base64_encode(const uint8_t*, size_t len);
std::vector<uint8_t> base64_decode(const uint8_t*, size_t len);

#endif /* BASE64_H_C0CE2A47_D10E_42C9_A27C_C883944E704A */
