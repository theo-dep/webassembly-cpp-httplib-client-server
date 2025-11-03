#include <httplib.h>

int main()
{
    httplib::Server svr;

    svr.Get("/hi", [](const httplib::Request&, httplib::Response& res) {
        res.set_content("Hello World!", "text/plain");
    });

    return svr.listen("localhost", 8080);
}
