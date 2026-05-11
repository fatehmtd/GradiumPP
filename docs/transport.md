# Transport Layer

The library keeps transport code separate from the client logic. REST clients depend on `IHttpTransport`, and streaming clients depend on `IWebSocketTransport`. You can pass your own implementations if you need to test against mocks or plug in a different networking stack.

## HTTP transport

`IHttpTransport` exposes a single `send()` method that takes an `HttpRequest` and returns an `HttpResponse`.

Behavior is intentionally simple:

- HTTP error responses are returned to the caller. Client code decides when to throw `GradiumApiException`.
- Transport implementations throw `std::runtime_error` for network, TLS, and other I/O failures.
- Thread safety is not required by the interface.

`HttpRequest` carries the request method, full URL, headers, content type, and one of the supported body formats. Raw audio uploads use `binary_body`. Voice uploads use `multipart_files`.

The default implementation is `CurlHttpTransport`.

- Windows uses libcurl with Schannel.
- Linux and macOS use libcurl with OpenSSL.

## WebSocket transport

`IWebSocketTransport` exposes `connect()`, `sendText()`, `sendBinary()`, `close()`, and `isOpen()`, along with the usual message and lifecycle callbacks.

The expected calling pattern is:

1. Register callbacks.
2. Call `connect()`.
3. Send messages after the handshake completes.

Additional behavior to keep in mind:

- `connect()` blocks until the HTTP upgrade handshake finishes.
- `sendText()`, `sendBinary()`, `close()`, and `isOpen()` are safe to call from different threads.
- Receive callbacks run on the transport's internal thread, so avoid sending new messages directly from inside a callback.

Platform defaults:

- Windows: `WinHttpWebSocketTransport`
- Linux/macOS: `LwsWebSocketTransport`

## Custom transports

You can inject your own transport implementations through the client constructors.

```cpp
auto mockHttp = std::make_shared<MyMockHttpTransport>();
gradium::TtsRestClient client("api-key", mockHttp);
```

```cpp
auto myWs = std::make_shared<MyWebSocketTransport>();
gradium::TtsRealtimeClient client(myWs);
```
