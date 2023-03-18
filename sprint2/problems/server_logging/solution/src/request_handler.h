#pragma once
#include <filesystem>
#include <iostream>
#include <memory>

#include "http_server.h"
#include "model.h"
#include "dto.h"
#include "file_utils.h"

#define BOOST_URL_NO_LIB
#include <boost/url.hpp>
#include <boost/json.hpp>

#define BOOST_BEAST_USE_STD_STRING_VIEW

namespace http_handler {
namespace beast = boost::beast;
namespace http = beast::http;
namespace url = boost::urls;
namespace fs = std::filesystem;
namespace sys = boost::system;
namespace json = boost::json;

using StringResponse = http::response<http::string_body>;
using EmptyResponse = http::response<http::empty_body>;
using FileResponse = http::response<http::file_body>;

using namespace std::literals;

std::string_view mime_type(fs::path path);

template <typename Body, typename Allocator>
http::message_generator BadRequest(const http::request<Body, http::basic_fields<Allocator>>& request, const std::string& message) {
    StringResponse response { http::status::bad_request, request.version()};
    response.set(http::field::content_type, "text/plain");
    response.keep_alive(request.keep_alive());
    response.body() = message;
    response.prepare_payload();
    return response;
};

template <typename Body, typename Allocator>
http::message_generator NotFound(const http::request<Body, http::basic_fields<Allocator>>& request, const std::string& message) {
    StringResponse response { http::status::not_found, request.version()};
    response.set(http::field::content_type, "text/plain");
    response.keep_alive(request.keep_alive());
    response.body() = message;
    response.prepare_payload();
    return response;
};

template <typename Body, typename Allocator>
http::message_generator InternalError(const http::request<Body, http::basic_fields<Allocator>>& request, const std::string& message) {
    StringResponse response { http::status::internal_server_error, request.version()};
    response.set(http::field::content_type, "text/plain");
    response.keep_alive(request.keep_alive());
    response.body() = message;
    response.prepare_payload();
    return response;
};

template <typename Body, typename Allocator, typename Object>
http::message_generator Json(
    const http::request<Body, http::basic_fields<Allocator>>& request, 
    Object object,
    http::status status_code = http::status::ok){
    auto jv = json::value_from(object);

    auto json_string = json::serialize(jv);

    StringResponse response {status_code, request.version()};
    response.set(http::field::content_type, "application/json");
    response.body() = json_string;
    response.keep_alive(request.keep_alive());
    response.prepare_payload();
    return response;
}

class StaticFileRequestHandler {
public:
    StaticFileRequestHandler(fs::path wwwroot);

    StaticFileRequestHandler(const StaticFileRequestHandler&) =delete;
    StaticFileRequestHandler& operator=(const StaticFileRequestHandler&) = delete;

    StaticFileRequestHandler(StaticFileRequestHandler&& other):
        wwwroot_{other.wwwroot_} {}

    template <typename Body, typename Allocator>
    http::message_generator operator()(http::request<Body, http::basic_fields<Allocator>>&& req) {
        if( req.method() != http::verb::get &&
            req.method() != http::verb::head)
            return BadRequest(req, "Method not allowed"s);

        if(req.target().empty() ||
            req.target()[0] != '/')
            return BadRequest(req, "Bad request"s);

        url::decode_view decoded_target(req.target());

        std::string decoded_path {decoded_target.begin(), decoded_target.end()};

        if (decoded_path.back() == '/')
        {
            decoded_path = decoded_path.append("index.html"s);
        }

        auto file_path = fs::weakly_canonical(wwwroot_ / decoded_path.substr(1));

        if (file_path.generic_string().back() == '/') {
            file_path = file_path / "index.html"s;
        }

        if (!IsSubPath(file_path, wwwroot_)){
            return BadRequest(req, "Incorrent path"s);
        }

        http::file_body::value_type file_body;

        sys::error_code ec;

        file_body.open(file_path.c_str(), beast::file_mode::scan, ec);

        if (ec == sys::errc::no_such_file_or_directory){
            return NotFound(req, "File not found"s);
        }

        if (ec) {
            return InternalError(req, ec.message());
        }

        auto const size = file_body.size();

        if (req.method() == http::verb::head){
            EmptyResponse response {http::status::ok, req.version()};
            response.set(http::field::content_type, mime_type(file_path));
            response.content_length(size);
            response.keep_alive(req.keep_alive());
            return response;
        }

        FileResponse response { 
            std::piecewise_construct,
            std::make_tuple(std::move(file_body)),
            std::make_tuple(http::status::ok, req.version())};

        response.set(http::field::content_type, mime_type(file_path));
        response.content_length(size);
        response.keep_alive(req.keep_alive());
        return response;
    }
private:
    fs::path wwwroot_;
};


class RequestHandler {
public:

    using JsonResponse = http::response<http::string_body>;
    using StatusCodeResponse = http::response<http::empty_body>;
    using FileResponse = http::response<http::file_body>;

    explicit RequestHandler(model::Game& game, StaticFileRequestHandler&& static_handler)
        : game_{game}, static_file_handler_(std::move(static_handler)) {}

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    RequestHandler(RequestHandler&& other) : 
        game_(other.game_),
        static_file_handler_(std::move(other.static_file_handler_))  {}

    template <typename Body, typename Allocator>
    http::message_generator operator()(http::request<Body, http::basic_fields<Allocator>>&& request) {
        if (!request.target().starts_with("/api/"s)){
            return static_file_handler_(std::move(request));
        }

        // отправить список карт
        if (request.target() == "/api/v1/maps"s){
            // получить список карт
            auto maps = game_.GetMaps();

            // спроецировать в DTO
            std::vector<dto::MapRegistryDto> maps_dto(maps.size());

            std::transform(maps.begin(), maps.end(), maps_dto.begin(), [](model::Map arg){return dto::MapRegistryDto(std::move(arg));});

            // отправить массив DTO
            return Json(request, maps_dto);
        }

        // отправить карту
        if (request.target().starts_with("/api/v1/maps/"s)){
            auto map_name = std::string(request.target().substr("/api/v1/maps/"s.size()));

            auto map = game_.FindMap(model::Map::Id{map_name});

            if (map == nullptr){
                return Json(request, dto::ErrorDto {"mapNotFound"s, "Map not found"s}, http::status::not_found);
            }

            return Json(request, map);
        }

        // отправить BadRequest
        return Json(request, dto::ErrorDto {"badRequest"s, "Bad request"s}, http::status::bad_request);
    }

private:
    model::Game& game_;
    StaticFileRequestHandler static_file_handler_;
};
}  // namespace http_handler
