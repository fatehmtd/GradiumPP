#pragma once

#include <gradiumpp/types.hpp>
#include <gradiumpp/transport/http_transport.hpp>

#include <nlohmann/json.hpp>

namespace gradium::detail {

inline std::string stringOrEmpty(const nlohmann::json& obj, const char* key)
{
    if (!obj.contains(key) || obj[key].is_null()) {
        return "";
    }
    if (obj[key].is_string()) {
        return obj[key].get<std::string>();
    }
    return obj[key].dump();
}

inline void throwIfError(const transport::HttpResponse& response,
                         const std::string& operation)
{
    if (response.status_code >= 400) {
        const std::string body(response.body.begin(), response.body.end());
        GradiumErrorDetail detail;
        detail.status_code = static_cast<int>(response.status_code);
        detail.raw_body    = body;

        const auto payload = nlohmann::json::parse(body, nullptr, false);
        if (!payload.is_discarded()) {
            detail.message = stringOrEmpty(payload, "message");
            detail.code    = stringOrEmpty(payload, "code");
        }

        throw GradiumApiException(operation, std::move(detail));
    }
}

} // namespace gradium::detail
