#ifndef BASE62_ENCODER_H
#define BASE62_ENCODER_H

#include <string>

class Base62Encoder {
public:
    // Encodes a non-negative 64-bit integer ID into a Base62 string.
    static std::string encode(long long number);

    // Decodes a Base62 string back into a 64-bit integer ID.
    static long long decode(const std::string& code);
};

#endif // BASE62_ENCODER_H
