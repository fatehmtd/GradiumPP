#pragma once

#include <gradiumpp/transport/websocket_transport.hpp>

#include <atomic>
#include <memory>
#include <mutex>

namespace gradium::transport {

struct LwsWebSocketTransportImpl;

class LwsWebSocketTransport final : public IWebSocketTransport {
public:
    LwsWebSocketTransport();
    ~LwsWebSocketTransport() override;

    void setOnOpen(OpenHandler handler) override;
    void setOnTextMessage(TextMessageHandler handler) override;
    void setOnBinaryMessage(BinaryMessageHandler handler) override;
    void setOnError(ErrorHandler handler) override;
    void setOnClose(CloseHandler handler) override;

    void connect(const WebSocketConnectOptions& options) override;
    void sendText(const std::string& message) override;
    void sendBinary(const std::vector<std::uint8_t>& payload) override;
    void close() override;
    bool isOpen() const override;

private:
    std::unique_ptr<LwsWebSocketTransportImpl> _impl;
};

} // namespace gradium::transport
