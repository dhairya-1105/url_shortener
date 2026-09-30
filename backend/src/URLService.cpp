#include "URLService.h"
#include <cctype>
#include <stdexcept>
#include <algorithm>

URLService::URLService(Database& db)
    : database(db) {}

bool URLService::isValidUrl(const std::string& url) {
    if (url.empty() || url.length() > 2048) {
        return false;
    }
    // Basic check for dot and minimum length
    if (url.find('.') == std::string::npos) {
        return false;
    }
    return true;
}

bool URLService::isValidAlias(const std::string& alias) {
    if (alias.empty() || alias.length() > 50) {
        return false;
    }
    for (char c : alias) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '-' && c != '_') {
            return false;
        }
    }
    return true;
}

std::string URLService::createURL(const std::string& originalURL, const std::string& customAlias) {
    if (!isValidUrl(originalURL)) {
        throw std::invalid_argument("Invalid URL format");
    }

    std::string formattedUrl = originalURL;
    if (formattedUrl.rfind("http://", 0) != 0 && formattedUrl.rfind("https://", 0) != 0) {
        formattedUrl = "http://" + formattedUrl;
    }

    if (!customAlias.empty()) {
        if (!isValidAlias(customAlias)) {
            throw std::invalid_argument("Invalid custom alias format");
        }

        URLRecord existing;
        if (database.getUrlByShortCode(customAlias, existing)) {
            throw std::runtime_error("ALIAS_EXISTS");
        }

        long long rowId = database.createUrlRecord(formattedUrl, customAlias);
        if (rowId == -1) {
            throw std::runtime_error("Database insert failed");
        }
        return customAlias;
    }

    // Auto-generate using Base62 from DB ID
    long long rowId = database.createUrlRecord(formattedUrl, "");
    if (rowId == -1) {
        throw std::runtime_error("Database insert failed");
    }

    std::string generatedCode = Base62Encoder::encode(rowId);
    if (!database.updateShortCode(rowId, generatedCode)) {
        throw std::runtime_error("Failed to update generated short code");
    }

    return generatedCode;
}

bool URLService::getURL(const std::string& shortCode, URLRecord& record) {
    return database.getUrlByShortCode(shortCode, record);
}

bool URLService::deleteURL(const std::string& shortCode) {
    return database.deleteUrl(shortCode);
}

std::vector<URLRecord> URLService::getAllURLs() {
    return database.getAllUrls();
}
