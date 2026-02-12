#include <httplib.h>

#include <print>

int main()
{
    httplib::Client cli("localhost", 8080);

#ifdef __EMSCRIPTEN__
    if (auto res = cli.connect_emscripten_websocket(); res != EMSCRIPTEN_RESULT_SUCCESS) {
        std::println("WebSocket error: {}", res);
    }
#endif

    std::println("client is now connecting to http://localhost:8080/");

    if (auto res = cli.Get("/hi")) {
        if (res->status == httplib::StatusCode::OK_200) {
            std::println("{}", res->body);
        } else {
            std::println("HTTP status: {}", httplib::status_message(res->status));
        }
    } else {
        std::println("HTTP error: {}", httplib::to_string(res.error()));
    }

    return 0;
}
