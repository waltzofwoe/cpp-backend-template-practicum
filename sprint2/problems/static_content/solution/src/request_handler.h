#pragma once
#include "http_server.h"
#include "model.h"
#include <filesystem>
#include <iostream>
#include "file_utils.h"

#define BOOST_URL_NO_LIB
#include <boost/url.hpp>

#define BOOST_BEAST_USE_STD_STRING_VIEW

namespace http_handler {
namespace beast = boost::beast;
namespace http = beast::http;
namespace url = boost::urls;
namespace fs = std::filesystem;
namespace sys = boost::system;

class RequestHandler {
public:

    using JsonResponse = http::response<http::string_body>;
    using StatusCodeResponse = http::response<http::empty_body>;
    using FileResponse = http::response<http::file_body>;

    explicit RequestHandler(model::Game& game)
        : game_{game} {
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        // Обработать запрос request и отправить ответ, используя send
        using namespace std::literals;

        if (!req.target().starts_with("/api/"s))
        {
            url::decode_view target_decoded(req.target().substr(1));

            fs::path target_path(target_decoded.begin(), target_decoded.end());

            fs::path root_path("../static"s);

            std::cout << root_path << std::endl;

            auto file_path = fs::weakly_canonical(root_path / target_path);

            std::cout << file_path << std::endl;

            // if (!IsSubPath(file_path, root_path))
            // {
            //     StatusCodeResponse bad_request_response(http::status::bad_request, req.version());

            //     bad_request_response.set(http::field::content_type, "text/plain"s);

            //     send(bad_request_response);

            //     return;
            // }

            // if (!fs::exists(file_path)) {
            //     StatusCodeResponse error_response(http::status::not_found, req.version());

            //     error_response.set(http::field::content_type, "text/plain"s);

            //     send(error_response);

            //     return;
            // }
                
            FileResponse file_response(http::status::ok, req.version());

            file_response.set(http::field::content_type, "application/octet-stream"s);

            http::file_body::value_type file;

            if (sys::error_code ec; file.open(file_path.c_str(), beast::file_mode::read, ec), ec) {
                std::cout << "Failed to open file "sv << file_path << std::endl;
                
                // StatusCodeResponse error_response(http::status::internal_server_error, req.version());

                // error_response.set(http::field::content_type, "text/plain"s);

                // send(error_response);

                return;
            }

            file_response.body() = std::move(file);

            file_response.prepare_payload();

            send(file_response);
        }

        /*
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
        */

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
