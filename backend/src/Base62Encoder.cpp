#include "Base62Encoder.h"
#include <algorithm>
#include <stdexcept>

static const std::string BASE62_CHARS = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

std::string Base62Encoder::encode(long long number) {
    if (number <= 0) {
        return "0";
    }

    std::string result;
    while (number > 0) {
        int remainder = number % 62;
        result.push_back(BASE62_CHARS[remainder]);
        number /= 62;
    }

    std::reverse(result.begin(), result.end());
    return result;
}

long long Base62Encoder::decode(const std::string& code) {
    if (code.empty()) {
        return 0;
    }

    long long result = 0;
    for (char c : code) {
        size_t index = BASE62_CHARS.find(c);
        if (index == std::string::npos) {
            throw std::invalid_argument("Invalid Base62 character");
        }
        result = result * 62 + static_cast<long long>(index);
    }

    return result;
}
