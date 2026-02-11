#include <httplib.h>

#include <print>

int main()
{
#ifdef __EMSCRIPTEN__
    httplib::emscripten_init_websocket("http://localhost:8080/ws");
#endif

    httplib::Client cli("localhost", 8080);

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
