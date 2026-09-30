#ifndef URL_SERVICE_H
#define URL_SERVICE_H

#include <string>
#include <vector>
#include "Database.h"
#include "Base62Encoder.h"

class URLService {
private:
    Database& database;

public:
    explicit URLService(Database& db);

    // Creates a short URL record and returns the shortCode.
    // Throws std::invalid_argument if URL or customAlias is invalid.
    // Throws std::runtime_error("ALIAS_EXISTS") if customAlias is already taken.
    std::string createURL(const std::string& originalURL, const std::string& customAlias = "");

    // Retrieves a URL record by shortCode.
    bool getURL(const std::string& shortCode, URLRecord& record);

    // Soft-deletes a URL by setting active = 0.
    bool deleteURL(const std::string& shortCode);

    // Retrieves all active URLs.
    std::vector<URLRecord> getAllURLs();

    // Validation helpers
    static bool isValidUrl(const std::string& url);
    static bool isValidAlias(const std::string& alias);
};

#endif // URL_SERVICE_H
