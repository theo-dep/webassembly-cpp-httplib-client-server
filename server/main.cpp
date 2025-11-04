#include <httplib.h>

#include <filesystem>

int main(int, char** argv)
{
    const std::filesystem::path current_path{ std::filesystem::path(argv[0]).parent_path() };
    httplib::Server svr;

    svr.Get("/", [&current_path](const httplib::Request&, httplib::Response& res) {
        res.set_file_content((current_path / "client.html").string());
    });

    svr.Get("/client.(js|wasm)", [&current_path](const httplib::Request& req, httplib::Response& res) {
        res.set_file_content((current_path / "client.").string() + req.matches[1].str());
    });

    svr.Get("/hi", [](const httplib::Request&, httplib::Response& res) {
        res.set_content("Hello World!", "text/plain");
    });

    return svr.listen("localhost", 8080);
}
