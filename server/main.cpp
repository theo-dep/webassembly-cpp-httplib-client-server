#include "connect_websocket.h"

#include <httplib.h>

#include <filesystem>
#include <print>

// Sends WebSocket handshake back to the given WebSocket connection.
void SendHandshake(const httplib::Request& req, int fd) {
    static constexpr std::string_view webSocketGlobalGuid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"; // 36 characters long
    std::string key = req.get_header_value("Sec-WebSocket-Key");
    key += webSocketGlobalGuid.data();

    char sha1[21];
    std::println("hashing key: \"{}\"", key);
    SHA1(sha1, key.data(), key.size());

    const std::string handshakeMsg =
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: " + httplib::detail::base64_encode(sha1) + "\r\n"
        "\r\n";

    int err = send(fd, handshakeMsg.data(), handshakeMsg.size(), 0);
    if (err < 0) {
        std::println(stderr, "Client write failed\n");
        return;
    }

    std::println("Sent handshake:\n{}\n", handshakeMsg);
}

int main(int, char** argv)
{
    initWebSocketSendLock();

    const std::filesystem::path current_path{ std::filesystem::path(argv[0]).parent_path() };

    std::println("server is now listening to http://localhost:8080/");

    const int res = httplib::Server()

        .set_post_routing_handler([](const httplib::Request&, httplib::Response& res) {
            // https://developer.chrome.com/blog/enabling-shared-array-buffer
            res.set_header("Cross-Origin-Embedder-Policy", "require-corp");
            res.set_header("Cross-Origin-Opener-Policy", "same-origin");
        })

        .set_error_logger([](const httplib::Error& err, const httplib::Request* req) {
            std::cerr << httplib::to_string(err) << " while processing request";
            if (req) {
                std::cerr << ", client: " << req->get_header_value("X-Forwarded-For")
                          << ", request: '" << req->method << " " << req->path << " " << req->version << "'"
                          << ", host: " << req->get_header_value("Host");
            }
            std::cerr << std::endl;
        })

        .Get("/", [&current_path](const httplib::Request&, httplib::Response& res) {
            res.set_file_content((current_path / "client.html").string());
        })

        .Get("/ws", [](const httplib::Request& req, httplib::Response& res) {
            std::println("try to connect to WebSocket connection");

            SendHandshake(req, req.client_sock);
            std::println("websocket_to_posix_proxy server is now listening to WebSocket connection");

            wait_websocket_client(req.client_sock);
        })

        .Get("/client.(js|wasm)", [&current_path](const httplib::Request& req, httplib::Response& res) {
            res.set_file_content((current_path / "client.").string() + req.matches[1].str());
        })

        .Get("/hi", [](const httplib::Request&, httplib::Response& res) {
            res.set_content("Hello World!", "text/plain");
        })

        .listen("localhost", 8080) ? EXIT_SUCCESS : EXIT_FAILURE;

    return res;
}
