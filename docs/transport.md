# GradiumPP — Transport Layer

GradiumPP separates networking concerns from protocol logic through two abstract interfaces.
All clients accept optional shared pointers to custom implementations, enabling testing with
mock transports or integration with alternative networking libraries.

## HTTP Transport (`IHttpTransport`)

```cpp
namespace gradium::transport {
    class IHttpTransport {
    public:
        virtual HttpResponse send(const HttpRequest& request) = 0;
    };
}
```

- **Does NOT throw on HTTP ≥ 400** — callers inspect `status_code` and throw `GradiumApiException`
- **Throws `std::runtime_error`** only on I/O or TLS failures (DNS, timeout, connection refused)
- Not required to be thread-safe

### `HttpRequest` fields

| Field           | Purpose                                      |
|-----------------|----------------------------------------------|
| `method`        | `Get`, `Post`, `Put`, or `Delete`            |
| `url`           | Full URL including query string if needed    |
| `headers`       | Additional headers (x-api-key is set here)   |
| `content_type`  | Sets `Content-Type` header                   |
| `body`          | JSON or text body (mutually exclusive with binary_body/multipart) |
| `binary_body`   | Raw audio bytes (used for ASR REST POST)     |
| `multipart_files` | File upload (used for voice creation)      |
| `timeout_ms`    | Request timeout in milliseconds (0 = default)|

### Platform implementation: `CurlHttpTransport`

Built from libcurl 8.11.1 via FetchContent. TLS backend:
- **Windows**: Schannel (no OpenSSL dependency)
- **Linux/macOS**: OpenSSL

## WebSocket Transport (`IWebSocketTransport`)

```cpp
namespace gradium::transport {
    class IWebSocketTransport {
    public:
        virtual void connect(const WebSocketConnectOptions& options) = 0;
        virtual void sendText(const std::string& message) = 0;
        virtual void sendBinary(const std::vector<uint8_t>& payload) = 0;
        virtual void close() = 0;
        virtual bool isOpen() const = 0;
        // setOnOpen, setOnTextMessage, setOnBinaryMessage, setOnError, setOnClose
    };
}
```

- Register all `setOn*` handlers **before** calling `connect()`
- `connect()` blocks until the HTTP upgrade handshake completes
- `sendText()` / `sendBinary()` / `close()` / `isOpen()` are thread-safe
- Handler callbacks fire from the internal receive-loop thread — do not call `sendText()` from inside a callback

### Platform implementations

| Platform | Class | Backend |
|----------|-------|---------|
| Windows  | `WinHttpWebSocketTransport` | WinHTTP (built-in, no extra deps) |
| Linux/macOS | `LwsWebSocketTransport` | libwebsockets 4.3.3 + OpenSSL |

CMake selects the correct implementation at configure time via `if(WIN32)`.

## Custom Transport Injection

```cpp
// Example: inject a mock HTTP transport for testing
auto mockHttp = std::make_shared<MyMockHttpTransport>();
gradium::TtsRestClient client("api-key", mockHttp);
```

```cpp
// Example: inject a custom WebSocket transport
auto myWs = std::make_shared<MyWebSocketTransport>();
gradium::TtsRealtimeClient client(myWs);
```
