#pragma once

#include <gradiumpp/types.hpp>
#include <gradiumpp/transport/http_transport.hpp>

#include <memory>
#include <string>
#include <vector>

namespace gradium {

struct VoiceCreateRequest {
    std::string name;                  ///< required
    std::string audio_file_path;       ///< required — local path to reference audio
    std::string description;
    std::string language;              ///< "en", "fr", "de", "es", "pt"
    std::string input_format;          ///< audio format; empty = auto-detect
    double      start_s{0.0};         ///< start offset in the audio file
    std::vector<std::string> tags;
};

struct VoiceUpdateRequest {
    std::string name;
    std::string description;
    std::string language;
    std::vector<std::string> tags;
};

/// CRUD client for Gradium voice management (/voices/).
class VoiceClient {
public:
    explicit VoiceClient(
        std::string api_key,
        std::shared_ptr<transport::IHttpTransport> http_transport = nullptr,
        std::string base_url = gradium::api::base_url);

    // GET /voices/
    std::string         listVoices(bool include_catalog = false);
    std::vector<Voice>  listVoicesTyped(bool include_catalog = false);

    // POST /voices/  (multipart/form-data with audio file)
    std::string createVoice(const VoiceCreateRequest& request);
    Voice       createVoiceTyped(const VoiceCreateRequest& request);

    // GET /voices/{uid}
    std::string getVoice(const std::string& uid);
    Voice       getVoiceTyped(const std::string& uid);

    // PUT /voices/{uid}
    std::string updateVoice(const std::string& uid, const VoiceUpdateRequest& request);
    Voice       updateVoiceTyped(const std::string& uid, const VoiceUpdateRequest& request);

    // DELETE /voices/{uid}
    void deleteVoice(const std::string& uid);

private:
    std::string _apiKey;
    std::string _baseUrl;
    std::shared_ptr<transport::IHttpTransport> _httpTransport;

    transport::HttpRequest makeRequest(
        transport::HttpMethod method,
        const std::string& path,
        const std::string& json_body = "") const;

    Voice parseVoice(const std::string& json) const;
};

} // namespace gradium
