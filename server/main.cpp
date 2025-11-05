#include "connect_websocket.h"

#include <httplib.h>

#include <filesystem>
#include <thread>

int main(int, char** argv)
{
    if (!connect_websocket_server(5000)) {
        return EXIT_FAILURE;
    }

    std::jthread websocket_worker(
        [](std::stop_token stoken)
        {
            while (true) {
                connect_websocket_client();

                using namespace std::chrono_literals;
                std::this_thread::sleep_for(300ms);

                if (stoken.stop_requested())
                    return;
            }
        });

    const std::filesystem::path current_path{ std::filesystem::path(argv[0]).parent_path() };

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

        //.Get("/ws", [](const httplib::Request&, httplib::Response& res) {
        //    if (!connect_websocket_client()) {
        //        res.status = httplib::StatusCode::InternalServerError_500;
        //    }
        //})

        .Get("/client.(js|wasm)", [&current_path](const httplib::Request& req, httplib::Response& res) {
            res.set_file_content((current_path / "client.").string() + req.matches[1].str());
        })

        .Get("/hi", [](const httplib::Request&, httplib::Response& res) {
            res.set_content("Hello World!", "text/plain");
        })

        .listen("localhost", 8080) ? EXIT_SUCCESS : EXIT_FAILURE;

    websocket_worker.request_stop();
    websocket_worker.join();

    disconnect_websocket_server();

    return res;
}
