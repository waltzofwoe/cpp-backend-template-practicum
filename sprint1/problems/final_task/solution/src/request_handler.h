#pragma once
#include "http_server.h"
#include "model.h"

#define BOOST_BEAST_USE_STD_STRING_VIEW

namespace http_handler {
namespace beast = boost::beast;
namespace http = beast::http;

class RequestHandler {
public:

    using JsonResponse = http::response<http::string_body>;
    using StatusCodeResponse = http::response<http::empty_body>;

    explicit RequestHandler(model::Game& game)
        : game_{game} {
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        // Обработать запрос request и отправить ответ, используя send
        using namespace std::literals;

        if (req.target() == "/api/v1/maps"){
            // отправить список карт
            auto maps = GetMapsJson();

            auto response = MakeJsonResponse(http::status::ok, req.version(), maps);

            send(response);

            return;
        }

        if (req.target().starts_with("/api/v1/maps/"s)){
            // отправить карту

            auto map_name = std::string(req.target().substr("/api/v1/maps/"s.size()));

            auto map = game_.FindMap(model::Map::Id{map_name});

            if (map == nullptr){
                auto notFound = MakeErrorResponse(http::status::not_found, req.version(), "mapNotFound"s, "Map not found"s);

                send(notFound);

                return;
            }

            auto map_json = GetMapJson(map);

            auto map_response = MakeJsonResponse(http::status::ok, req.version(), map_json);

            send(map_response);

            return;
        }

        auto bad_request = MakeErrorResponse(http::status::bad_request, req.version(), "badRequest"s, "Bad request");

        send(bad_request);
    }

private:
    model::Game& game_;

    std::string GetMapsJson();

    std::string GetMapJson(const model::Map* map);

    JsonResponse MakeJsonResponse(http::status status, unsigned http_version, std::string_view response);

    JsonResponse MakeErrorResponse(http::status status, unsigned http_version, std::string_view code, std::string_view message);
};

}  // namespace http_handler
