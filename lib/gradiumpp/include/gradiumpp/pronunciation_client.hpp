#pragma once

#include <gradiumpp/types.hpp>
#include <gradiumpp/transport/http_transport.hpp>

#include <memory>
#include <string>
#include <vector>

namespace gradium {

struct PronunciationCreateRequest {
    std::string name;        ///< required
    std::string language;    ///< required; "en", "fr", "de", "es", "pt"
    std::string description;
    std::vector<PronunciationRule> rules;
};

struct PronunciationUpdateRequest {
    std::string name;
    std::string description;
    std::vector<PronunciationRule> rules;
};

/// CRUD client for Gradium pronunciation dictionaries (/pronunciations/).
class PronunciationClient {
public:
    explicit PronunciationClient(
        std::string api_key,
        std::shared_ptr<transport::IHttpTransport> http_transport = nullptr,
        std::string base_url = gradium::api::base_url);

    // GET /pronunciations/
    std::string                          listDictionaries();
    std::vector<PronunciationDictionary> listDictionariesTyped();

    // POST /pronunciations/
    std::string             createDictionary(const PronunciationCreateRequest& request);
    PronunciationDictionary createDictionaryTyped(const PronunciationCreateRequest& request);

    // GET /pronunciations/{uid}
    std::string             getDictionary(const std::string& uid);
    PronunciationDictionary getDictionaryTyped(const std::string& uid);

    // PUT /pronunciations/{uid}
    std::string             updateDictionary(const std::string& uid,
                                             const PronunciationUpdateRequest& request);
    PronunciationDictionary updateDictionaryTyped(const std::string& uid,
                                                   const PronunciationUpdateRequest& request);

    // DELETE /pronunciations/{uid}
    void deleteDictionary(const std::string& uid);

private:
    std::string _apiKey;
    std::string _baseUrl;
    std::shared_ptr<transport::IHttpTransport> _httpTransport;

    transport::HttpRequest makeRequest(
        transport::HttpMethod method,
        const std::string& path,
        const std::string& json_body = "") const;

    PronunciationDictionary parseDictionary(const std::string& json) const;
};

} // namespace gradium
