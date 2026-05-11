#pragma once

#include <gradiumpp/transport/websocket_transport.hpp>

#include <atomic>
#include <memory>
#include <mutex>

namespace gradium::transport {

class WinHttpWebSocketTransport final : public IWebSocketTransport {
public:
    WinHttpWebSocketTransport();
    ~WinHttpWebSocketTransport() override;

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
    struct Impl;
    std::unique_ptr<Impl> _impl;
    std::atomic<bool>     _isOpen{false};
    mutable std::mutex    _callbackMutex;
    OpenHandler           _onOpen;
    TextMessageHandler    _onText;
    BinaryMessageHandler  _onBinary;
    ErrorHandler          _onError;
    CloseHandler          _onClose;
};

} // namespace gradium::transport
