#pragma once

#include "api_constants.hpp"
#include "voice_constants.hpp"
#include "types.hpp"
#include "tts_client.hpp"
#include "asr_client.hpp"
#include "voice_client.hpp"
#include "pronunciation_client.hpp"
#include "credits_client.hpp"
#include "transport/http_transport.hpp"
#include "transport/websocket_transport.hpp"
#include "transport/curl_http_transport.hpp"

#ifdef _WIN32
#  include "transport/winhttp_websocket_transport.hpp"
#else
#  include "transport/lws_websocket_transport.hpp"
#endif
